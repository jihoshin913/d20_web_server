#include "not_found_handler.h"

HttpResponse NotFoundHandler::handle_request(const HttpRequest& request) {
    HttpResponse response = HttpResponse(
        "HTTP/1.1", 404, "Not Found",
        {{"Content-Type", "text/html"}},
        "<h1>404 Not Found</h1>"
    );

    return response;
}

std::string NotFoundHandler::get_handler_name() const {
    return "NotFoundHandler";
}
