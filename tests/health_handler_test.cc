#include "health_handler.h"
#include "http_request.h"
#include "http_response.h"
#include <gtest/gtest.h>

class HealthHandlerTest : public ::testing::Test {
protected:
    void SetUp() override {
        handler = std::make_unique<HealthHandler>();
    }
    
    std::unique_ptr<HealthHandler> handler;
};

// Test basic health check functionality with GET
TEST_F(HealthHandlerTest, Returns200AndOK) {
    std::string raw_request = 
        "GET /health HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "\r\n";
    
    HttpRequest request = HttpRequest::parse(raw_request);
    HttpResponse response = handler->handle_request(request);
    
    EXPECT_EQ(response.get_status_code(), 200);
    EXPECT_EQ(response.get_reason_phrase(), "OK");
    EXPECT_EQ(response.get_message_body(), "OK");
}

// Test response has correct Content-Type
TEST_F(HealthHandlerTest, SetsContentTypePlainText) {
    std::string raw_request = 
        "GET /health HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "\r\n";
    
    HttpRequest request = HttpRequest::parse(raw_request);
    HttpResponse response = handler->handle_request(request);
    
    EXPECT_EQ(response.get_header("Content-Type"), "text/plain");
}

// Test that HealthHandler ignores the request method (works for POST)
TEST_F(HealthHandlerTest, IgnoresRequestMethod) {
    std::string raw_request = 
        "POST /health HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "\r\n";
    
    HttpRequest request = HttpRequest::parse(raw_request);
    HttpResponse response = handler->handle_request(request);
    
    // Should still return 200 OK
    EXPECT_EQ(response.get_status_code(), 200);
    EXPECT_EQ(response.get_message_body(), "OK");
}

// Test that HealthHandler ignores the request body
TEST_F(HealthHandlerTest, IgnoresRequestBody) {
    std::string raw_request = 
        "POST /health HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "Content-Length: 5\r\n"
        "\r\n"
        "Trash";
    
    HttpRequest request = HttpRequest::parse(raw_request);
    HttpResponse response = handler->handle_request(request);
    
    // The response body should not be "Trash", it should remain "OK"
    EXPECT_EQ(response.get_message_body(), "OK");
}

// Test correct HTTP version in response
TEST_F(HealthHandlerTest, UsesHttp11Version) {
    std::string raw_request = 
        "GET /health HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "\r\n";
    
    HttpRequest request = HttpRequest::parse(raw_request);
    HttpResponse response = handler->handle_request(request);
    
    EXPECT_EQ(response.get_version(), "HTTP/1.1");
}

// Test correct Handler Name
TEST_F(HealthHandlerTest, ReturnsCorrectHandlerName) {
    EXPECT_EQ(handler->get_handler_name(), "HealthHandler");
}

// Test response conversion to string contains expected elements
TEST_F(HealthHandlerTest, ResponseCanBeConvertedToString) {
    std::string raw_request = 
        "GET /health HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "\r\n";
    
    HttpRequest request = HttpRequest::parse(raw_request);
    HttpResponse response = handler->handle_request(request);
    
    std::string response_str = response.convert_to_string();
    
    // Should have status line
    EXPECT_TRUE(response_str.find("HTTP/1.1 200 OK") != std::string::npos);
    // Should have Content-Type header
    EXPECT_TRUE(response_str.find("Content-Type: text/plain") != std::string::npos);
    // Should have the OK body
    EXPECT_TRUE(response_str.find("OK") != std::string::npos);
}