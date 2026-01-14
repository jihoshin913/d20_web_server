#include "server.h"
#include "logger.h"
#include <boost/bind.hpp>

using boost::asio::ip::tcp;

Server::Server(boost::asio::io_service& io_service, short port,
               std::shared_ptr<PathRouter> router)
  : io_service_(io_service),
    acceptor_(io_service, tcp::endpoint(tcp::v4(), port)),
    router_(router)
{
  start_accept();
}

void Server::start_accept()
{
  std::shared_ptr<Session> new_session = 
      std::make_shared<Session>(io_service_, router_);
  
  acceptor_.async_accept(new_session->socket(),
      boost::bind(&Server::handle_accept, this, new_session,
        boost::asio::placeholders::error));
}

void Server::handle_accept(std::shared_ptr<Session> new_session,
    const boost::system::error_code& error)
{
  if (!error)
  {
    new_session->start();
  }
  start_accept();
}