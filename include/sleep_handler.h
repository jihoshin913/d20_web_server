#ifndef SLEEP_HANDLER_H
#define SLEEP_HANDLER_H

#include "request_handler.h"
#include <chrono>

class SleepHandler : public RequestHandler {
public:
    explicit SleepHandler(int sleep_seconds = 5);
    
    HttpResponse handle_request(const HttpRequest& request) override;
    
    std::string get_handler_name() const override;

private:
    int sleep_seconds_;
};

#endif // SLEEP_HANDLER_H