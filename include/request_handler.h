#ifndef REQUEST_HANDLER_H 
#define REQUEST_HANDLER_H 

#include "http_request.h"
#include "http_response.h"
#include <string>
#include <unordered_set>

class RequestHandler {

  public:
    virtual ~RequestHandler() = default;

    virtual HttpResponse handle_request(const HttpRequest& request) = 0;

    virtual std::string get_handler_name() const = 0;

};

#endif
