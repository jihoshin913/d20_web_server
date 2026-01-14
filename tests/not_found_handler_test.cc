#include "not_found_handler.h"
#include "http_request.h"
#include <gtest/gtest.h>

TEST(NotFoundHandlerTest, Returns404) {
    NotFoundHandler handler;
    HttpRequest request;  // empty request
    HttpResponse response = handler.handle_request(request);

    EXPECT_EQ(response.get_version(), "HTTP/1.1");
    EXPECT_EQ(response.get_status_code(), 404);
    EXPECT_EQ(response.get_reason_phrase(), "Not Found");
    EXPECT_EQ(response.get_message_body(), "<h1>404 Not Found</h1>");

    EXPECT_EQ(response.get_header("Content-Type"), "text/html");
}

TEST(NotFoundHandlerTest, HandlerName) {
    NotFoundHandler handler;
    EXPECT_EQ(handler.get_handler_name(), "NotFoundHandler");
}

TEST(NotFoundHandlerTest, IgnoresRequestPath) {
    NotFoundHandler handler;
    HttpRequest request;
    request.set_path("/some/nonexistent/path");
    HttpResponse response = handler.handle_request(request);

    EXPECT_EQ(response.get_status_code(), 404);
    EXPECT_EQ(response.get_reason_phrase(), "Not Found");
}

TEST(NotFoundHandlerTest, MultipleRequests) {
    NotFoundHandler handler;
    HttpRequest request1, request2;
    request1.set_path("/first");
    request2.set_path("/second");

    HttpResponse response1 = handler.handle_request(request1);
    HttpResponse response2 = handler.handle_request(request2);

    EXPECT_EQ(response1.get_status_code(), 404);
    EXPECT_EQ(response2.get_status_code(), 404);
    EXPECT_EQ(response1.get_message_body(), response2.get_message_body());
}
