#include "sleep_handler.h"
#include <thread>
#include <sstream>

SleepHandler::SleepHandler(int sleep_seconds)
    : sleep_seconds_(sleep_seconds) {
}

HttpResponse SleepHandler::handle_request(const HttpRequest& request) {
    // Block for the specified duration
    std::this_thread::sleep_for(std::chrono::seconds(sleep_seconds_));
    
    // Create response body
    std::ostringstream body;
    body << "Slept for " << sleep_seconds_ << " seconds";
    
    // Return 200 OK with message
    return HttpResponse(
        "HTTP/1.1",
        200,
        "OK",
        {{"Content-Type", "text/plain"}},
        body.str()
    );
}

std::string SleepHandler::get_handler_name() const {
    return "SleepHandler";
}