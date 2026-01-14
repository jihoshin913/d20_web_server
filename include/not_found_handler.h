#ifndef NOT_FOUND_HANDLER_H
#define NOT_FOUND_HANDLER_H

// Class skeleton made by Chat GPT

#include "request_handler.h"
#include "http_response.h"

class NotFoundHandler : public RequestHandler {

    public:
        HttpResponse handle_request(const HttpRequest& request) override;
    
        std::string get_handler_name() const override;

};

#endif
