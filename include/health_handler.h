#ifndef HEALTH_HANDLER_H
#define HEALTH_HANDLER_H

#include "http_request.h"
#include "http_response.h"
#include "request_handler.h"

// This class handles the incoming request with 200 OK message, designed to help test the server health status
class HealthHandler : public RequestHandler {

  public:
    HttpResponse handle_request(const HttpRequest& request) override;

    std::string get_handler_name() const override;

};

#endif
