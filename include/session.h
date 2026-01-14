#ifndef SESSION_H
#define SESSION_H

#include <boost/asio.hpp>
#include <boost/enable_shared_from_this.hpp>
#include "path_router.h"
#include <string>
#include <memory>

using boost::asio::ip::tcp;

class Session : public std::enable_shared_from_this<Session>
{
public:
  Session(boost::asio::io_service& io_service,
          std::shared_ptr<PathRouter> router);

  tcp::socket& socket();

  void start();

private:
  void handle_read(const boost::system::error_code& error,
      size_t bytes_transferred);

  void handle_write(const boost::system::error_code& error);

  tcp::socket socket_;
  enum { max_length = 1024 };
  char data_[max_length];

  std::string buffer_;

  std::shared_ptr<PathRouter> router_;
};

#endif