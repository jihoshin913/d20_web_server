// echo_handler_test.cpp
#include "echo_handler.h"
#include "http_request.h"
#include "http_response.h"
#include <gtest/gtest.h>

class EchoHandlerTest : public ::testing::Test {
protected:
    void SetUp() override {
        handler = std::make_unique<EchoHandler>();
    }
    
    std::unique_ptr<EchoHandler> handler;
};

// Test basic echo functionality
TEST_F(EchoHandlerTest, EchoesRawRequest) {
    std::string raw_request = 
        "GET /echo/hello HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "\r\n";
    
    HttpRequest request = HttpRequest::parse(raw_request);
    HttpResponse response = handler->handle_request(request);
    
    EXPECT_EQ(response.get_status_code(), 200);
    EXPECT_EQ(response.get_reason_phrase(), "OK");
    EXPECT_EQ(response.get_message_body(), raw_request);
}

// Test echoes complete request with headers
TEST_F(EchoHandlerTest, EchoesCompleteRequestWithHeaders) {
    std::string raw_request = 
        "GET /echo/test HTTP/1.1\r\n"
        "Host: localhost:8080\r\n"
        "User-Agent: Mozilla/5.0\r\n"
        "Accept: text/html\r\n"
        "\r\n";
    
    HttpRequest request = HttpRequest::parse(raw_request);
    HttpResponse response = handler->handle_request(request);
    
    EXPECT_EQ(response.get_message_body(), raw_request);
    EXPECT_TRUE(response.get_message_body().find("Host: localhost:8080") != std::string::npos);
    EXPECT_TRUE(response.get_message_body().find("User-Agent: Mozilla/5.0") != std::string::npos);
}

// Test response has correct Content-Type
TEST_F(EchoHandlerTest, SetsContentTypePlainText) {
    std::string raw_request = 
        "GET /echo HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "\r\n";
    
    HttpRequest request = HttpRequest::parse(raw_request);
    HttpResponse response = handler->handle_request(request);
    
    EXPECT_EQ(response.get_header("Content-Type"), "text/plain");
}

// Test response has correct HTTP version
TEST_F(EchoHandlerTest, UsesHttp11Version) {
    std::string raw_request = 
        "GET /echo HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "\r\n";
    
    HttpRequest request = HttpRequest::parse(raw_request);
    HttpResponse response = handler->handle_request(request);
    
    EXPECT_EQ(response.get_version(), "HTTP/1.1");
}

// Test with POST request
TEST_F(EchoHandlerTest, EchoesPOSTRequest) {
    std::string raw_request = 
        "POST /echo HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "Content-Type: application/json\r\n"
        "Content-Length: 18\r\n"
        "\r\n"
        "{\"test\": \"data\"}";
    
    HttpRequest request = HttpRequest::parse(raw_request);
    HttpResponse response = handler->handle_request(request);
    
    EXPECT_EQ(response.get_message_body(), raw_request);
    EXPECT_TRUE(response.get_message_body().find("{\"test\": \"data\"}") != std::string::npos);
}

// Test with empty body request
TEST_F(EchoHandlerTest, EchoesRequestWithNoBody) {
    std::string raw_request = 
        "GET /echo/test HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "\r\n";
    
    HttpRequest request = HttpRequest::parse(raw_request);
    HttpResponse response = handler->handle_request(request);
    
    EXPECT_EQ(response.get_message_body(), raw_request);
    EXPECT_FALSE(response.get_message_body().empty());
}

// Test with special characters in path
TEST_F(EchoHandlerTest, EchoesRequestWithSpecialCharacters) {
    std::string raw_request = 
        "GET /echo/hello%20world HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "\r\n";
    
    HttpRequest request = HttpRequest::parse(raw_request);
    HttpResponse response = handler->handle_request(request);
    
    EXPECT_TRUE(response.get_message_body().find("/echo/hello%20world") != std::string::npos);
}

// Test Content-Length is automatically set
TEST_F(EchoHandlerTest, SetsContentLengthAutomatically) {
    std::string raw_request = 
        "GET /echo HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "\r\n";
    
    HttpRequest request = HttpRequest::parse(raw_request);
    HttpResponse response = handler->handle_request(request);
    
    std::string expected_length = std::to_string(raw_request.length());
    EXPECT_EQ(response.get_header("Content-Length"), expected_length);
}

// Test with Unix line endings
TEST_F(EchoHandlerTest, EchoesUnixLineEndings) {
    std::string raw_request = 
        "GET /echo HTTP/1.1\n"
        "Host: localhost\n"
        "\n";
    
    HttpRequest request = HttpRequest::parse(raw_request);
    HttpResponse response = handler->handle_request(request);
    
    EXPECT_EQ(response.get_message_body(), raw_request);
}

// Test with multiple line request
TEST_F(EchoHandlerTest, EchoesMultiLineRequest) {
    std::string raw_request = 
        "GET /echo/foo/bar/baz HTTP/1.1\r\n"
        "Host: localhost:8080\r\n"
        "Connection: keep-alive\r\n"
        "Accept: */*\r\n"
        "User-Agent: Test\r\n"
        "\r\n";
    
    HttpRequest request = HttpRequest::parse(raw_request);
    HttpResponse response = handler->handle_request(request);
    
    EXPECT_EQ(response.get_message_body(), raw_request);
    // Verify all headers are in the echoed body
    EXPECT_TRUE(response.get_message_body().find("Host: localhost:8080") != std::string::npos);
    EXPECT_TRUE(response.get_message_body().find("Connection: keep-alive") != std::string::npos);
    EXPECT_TRUE(response.get_message_body().find("Accept: */*") != std::string::npos);
}

// Test response can be converted to string
TEST_F(EchoHandlerTest, ResponseCanBeConvertedToString) {
    std::string raw_request = 
        "GET /echo HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "\r\n";
    
    HttpRequest request = HttpRequest::parse(raw_request);
    HttpResponse response = handler->handle_request(request);
    
    std::string response_str = response.convert_to_string();
    
    // Should have status line
    EXPECT_TRUE(response_str.find("HTTP/1.1 200 OK") != std::string::npos);
    // Should have Content-Type header
    EXPECT_TRUE(response_str.find("Content-Type: text/plain") != std::string::npos);
    // Should have the echoed body
    EXPECT_TRUE(response_str.find(raw_request) != std::string::npos);
}

// Test with very long request
TEST_F(EchoHandlerTest, EchoesLongRequest) {
    std::string long_path(1000, 'x');
    std::string raw_request = 
        "GET /echo/" + long_path + " HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "\r\n";
    
    HttpRequest request = HttpRequest::parse(raw_request);
    HttpResponse response = handler->handle_request(request);
    
    EXPECT_EQ(response.get_message_body(), raw_request);
    EXPECT_EQ(response.get_message_body().length(), raw_request.length());
}

// Test status code is always 200
TEST_F(EchoHandlerTest, AlwaysReturns200) {
    std::vector<std::string> requests = {
        "GET /echo HTTP/1.1\r\n\r\n",
        "POST /echo HTTP/1.1\r\n\r\n",
        "PUT /echo HTTP/1.1\r\n\r\n",
        "DELETE /echo HTTP/1.1\r\n\r\n"
    };
    
    for (const auto& raw_request : requests) {
        HttpRequest request = HttpRequest::parse(raw_request);
        HttpResponse response = handler->handle_request(request);
        EXPECT_EQ(response.get_status_code(), 200);
        EXPECT_EQ(response.get_reason_phrase(), "OK");
    }
}

// Test echoes exact raw request (byte-for-byte)
TEST_F(EchoHandlerTest, EchoesExactRawRequest) {
    std::string raw_request = 
        "GET /echo/test HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "X-Custom-Header: value\r\n"
        "\r\n";
    
    HttpRequest request = HttpRequest::parse(raw_request);
    HttpResponse response = handler->handle_request(request);
    
    // Body should be EXACTLY the raw request
    EXPECT_EQ(response.get_message_body(), raw_request);
    EXPECT_EQ(response.get_message_body().size(), raw_request.size());
}