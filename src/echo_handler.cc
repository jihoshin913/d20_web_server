#include "http_response.h"
#include "echo_handler.h"

HttpResponse EchoHandler::handle_request(const HttpRequest& request){

  HttpResponse response = HttpResponse("HTTP/1.1", 200, "OK", 
                                        {{"Content-Type", "text/plain"}},
                                        request.raw_request());

  return response;

}

std::string EchoHandler::get_handler_name() const {
  return "EchoHandler";
}
