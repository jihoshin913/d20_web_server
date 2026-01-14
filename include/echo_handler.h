#ifndef ECHO_HANDLER_H
#define ECHO_HANDLER_H

#include "http_request.h"
#include "http_response.h"
#include "request_handler.h"

// This class handles any requests with "echo" in the path.
// This implements the functionality for the webserver from 
// Assignment 2. 
// Inherits from the abstract request_handler class.
class EchoHandler : public RequestHandler {

  public:
    HttpResponse handle_request(const HttpRequest& request) override;

    std::string get_handler_name() const override;

};

#endif
