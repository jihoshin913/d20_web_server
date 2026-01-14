#ifndef SERVER_H
#define SERVER_H

#include "session.h"
#include "path_router.h"
#include <boost/asio.hpp>
#include <memory>

using boost::asio::ip::tcp;

class Server
{
public:
  Server(boost::asio::io_service& io_service, short port,
         std::shared_ptr<PathRouter> router);

private:
  void start_accept();

  void handle_accept(std::shared_ptr<Session> new_session,
      const boost::system::error_code& error);

  boost::asio::io_service& io_service_;
  tcp::acceptor acceptor_;

  std::shared_ptr<PathRouter> router_;
};

#endif