#include "session.h"
#include "http_request.h"
#include "http_helper.h"
#include "path_router.h"
#include "echo_handler.h"
#include "file_handler.h"
#include "logger.h"
#include <boost/bind.hpp>

Session::Session(boost::asio::io_service& io_service,
                 std::shared_ptr<PathRouter> router)
  : socket_(io_service), router_(router)
{
}

tcp::socket& Session::socket()
{
  return socket_;
}

void Session::start()
{
  auto self = shared_from_this();
  socket_.async_read_some(boost::asio::buffer(data_, max_length),
      boost::bind(&Session::handle_read, self,
        boost::asio::placeholders::error,
        boost::asio::placeholders::bytes_transferred));
}

void Session::handle_read(const boost::system::error_code& error,
    size_t bytes_transferred)
{
  if (!error)
  {
    // Accumulate bytes into buffer
    buffer_.append(data_, bytes_transferred);


    MalformedType malformed = check_malformed_request(buffer_);
    if (malformed != MalformedType::None) {
         HttpResponse response("HTTP/1.1", 400, "Bad Request",
             {{"Content-Type", "text/plain"}}, "Malformed HTTP request");

         HttpRequest request = HttpRequest::parse(buffer_);
         Logger * logger = Logger::getLogger();
    
         std::string response_str = response.convert_to_string();
    
         auto self = shared_from_this();
         boost::asio::async_write(socket_,
             boost::asio::buffer(response_str),
             boost::bind(&Session::handle_write, self,
                 boost::asio::placeholders::error));

         std::string client_ip = "unknown";
         try {
             client_ip = socket_.remote_endpoint().address().to_string();
         } catch (...) {
             // Socket might be closed, use "unknown"
         }
         logger->logMachineParsable("[ResponseMetrics] response_code:400 path:" + request.path() + 
                   " handler:MalformedRequest ip:" + client_ip);
    
         buffer_.clear();
         return;
    }

    // \n\n for macOS
    if (detect_http_request(buffer_)) {
      
      HttpRequest request = HttpRequest::parse(buffer_);

      // Large inputs converted to chunks, aggregate chunks into single request
      // Note: following code until (~129) written with assistance from Claude AI
      auto content_length_opt = request.get_header("Content-Length");
      if (content_length_opt.has_value()) {
        size_t content_length = 0;
        try {
          content_length = std::stoull(content_length_opt.value());
        } catch (const std::exception& e) {
          // Invalid Content-Length header
          Logger * logger = Logger::getLogger();
          logger->logDebugFile("Invalid Content-Length header");
          
          HttpResponse response = HttpResponse("HTTP/1.1", 400, "Bad Request",
              {{"Content-Type", "text/plain"}}, "Invalid Content-Length header");
          
          std::string response_str = response.convert_to_string();
          
          auto self = shared_from_this();
          boost::asio::async_write(socket_,
            boost::asio::buffer(response_str),
            boost::bind(&Session::handle_write, self,
              boost::asio::placeholders::error));

          std::string client_ip = "unknown";
          try {
              client_ip = socket_.remote_endpoint().address().to_string();
          } catch (...) {}
          logger->logMachineParsable("[ResponseMetrics] response_code:400 path:" + request.path() + 
                        " handler:MalformedRequest ip:" + client_ip);
          
          buffer_.clear();
          return;
        }
        
        size_t header_end = buffer_.find("\r\n\r\n");
        if (header_end == std::string::npos) {
          header_end = buffer_.find("\n\n");
        }
        
        size_t body_start = (buffer_.find("\r\n\r\n") != std::string::npos) 
                            ? header_end + 4 
                            : header_end + 2;
        size_t body_received = buffer_.size() - body_start;
        
        if (body_received < content_length) {
          auto self = shared_from_this();
          socket_.async_read_some(boost::asio::buffer(data_, max_length),
              boost::bind(&Session::handle_read, self,
                boost::asio::placeholders::error,
                boost::asio::placeholders::bytes_transferred));
          Logger * logger = Logger::getLogger();
          logger->logDebugFile("Waiting for complete body: " + 
                              std::to_string(body_received) + "/" + 
                              std::to_string(content_length) + " bytes");
          return;
        }
      }

      if(!request.is_valid()){
          Logger * logger = Logger::getLogger();
          logger->logDebugFile("Received malformed HTTP request");
          
          HttpResponse response = HttpResponse("HTTP/1.1", 400, "Bad Request",
              {{"Content-Type", "text/plain"}}, "Malformed HTTP request");
          
          std::string response_str = response.convert_to_string();
          
          auto self = shared_from_this();
          boost::asio::async_write(socket_,
            boost::asio::buffer(response_str),
            boost::bind(&Session::handle_write, self,
              boost::asio::placeholders::error));

          std::string client_ip = "unknown";
          try {
              client_ip = socket_.remote_endpoint().address().to_string();
          } catch (...) {
              // Socket might be closed, use "unknown"
          }
          logger->logMachineParsable("[ResponseMetrics] response_code:400 path:" + request.path() + 
                         " handler:MalformedRequest ip:" + client_ip);
          
          buffer_.clear();
          return;
      }

      Logger * logger = Logger::getLogger();

      std::unique_ptr<RequestHandler> handler = router_->match_handler(request.path());

      HttpResponse response;
      std::string handler_name = "NotFoundHandler";

      if (handler) {  
        handler_name = handler->get_handler_name();
        logger->logDebugFile("Request for path '" + request.path() + "' is being handled by " + handler->get_handler_name());
        response = handler->handle_request(request);
      } else {
        // Fallback for unknown paths
        logger->logDebugFile("No handler found for path: " + request.path());
        response = HttpResponse("HTTP/1.1", 404, "Not Found", 
          {{"Content-Type", "text/html"}}, "<h1>404 Not Found</h1>");
      }

      std::string response_str = response.convert_to_string();

      auto self = shared_from_this();
      boost::asio::async_write(socket_,
        boost::asio::buffer(response_str),
        boost::bind(&Session::handle_write, self,
          boost::asio::placeholders::error));

      std::string client_ip = "unknown";
      try {
          client_ip = socket_.remote_endpoint().address().to_string();
      } catch (...) {
          // Socket might be closed, use "unknown"
      }
      logger->logMachineParsable("[ResponseMetrics] response_code:" + std::to_string(response.get_status_code()) + 
                     " path:" + request.path() + " handler:" + handler_name + " ip:" + client_ip);

      buffer_.clear();

    } else {
      // keep reading if empty line not found
      auto self = shared_from_this();
      socket_.async_read_some(boost::asio::buffer(data_, max_length),
          boost::bind(&Session::handle_read, self,
            boost::asio::placeholders::error,
            boost::asio::placeholders::bytes_transferred));
      Logger * logger = Logger::getLogger();
      logger->logDebugFile("Read handler ");
    }
  }
}

void Session::handle_write(const boost::system::error_code& error)
{
  if (!error)
  {
    auto self = shared_from_this();
    socket_.async_read_some(boost::asio::buffer(data_, max_length),
        boost::bind(&Session::handle_read, self,
          boost::asio::placeholders::error,
          boost::asio::placeholders::bytes_transferred));
        Logger * logger = Logger::getLogger();
        logger->logDebugFile("write handler");
  }
}