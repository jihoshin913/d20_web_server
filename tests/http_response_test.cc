#include "gtest/gtest.h"
#include "http_response.h"

class HttpResponseTest : public testing::Test {
protected:
    HttpResponse response;
};

TEST_F(HttpResponseTest, DefaultConstructor) {
    HttpResponse resp;

    EXPECT_EQ(resp.get_version(), "HTTP/1.1");
    EXPECT_EQ(resp.get_message_body(), "");
    EXPECT_EQ(resp.get_header("Content-Length"), "0");
}

TEST_F(HttpResponseTest, SetVersion) {
    HttpResponse resp;
    resp.set_version("HTTP/1.0");
    EXPECT_EQ(resp.get_version(), "HTTP/1.0");
}

TEST_F(HttpResponseTest, SetStatusCodeAndReason) {
    HttpResponse resp;
    resp.set_status_code(404);
    resp.set_reason_phrase("Not Found");
    EXPECT_EQ(resp.get_status_code(), 404);
    EXPECT_EQ(resp.get_reason_phrase(), "Not Found");
}

TEST_F(HttpResponseTest, SetHeader) {
    HttpResponse resp;
    resp.set_header("Content-Type", "image/jpeg");
    EXPECT_EQ(resp.get_header("Content-Type"), "image/jpeg");
}

TEST_F(HttpResponseTest, SetHeaderOverwritesExisting) {
    HttpResponse resp;
    resp.set_header("Content-Type", "text/html");
    EXPECT_EQ(resp.get_header("Content-Type"), "text/html");

    resp.set_header("Content-Type", "application/json");
    EXPECT_EQ(resp.get_header("Content-Type"), "application/json");
}

TEST_F(HttpResponseTest, GetHeaderNotSet) {
    HttpResponse resp;
    EXPECT_EQ(resp.get_header("Non-Existent-Header"), "");
}

TEST_F(HttpResponseTest, SetMessageBodyUpdatesContentLength) {
    HttpResponse resp;
    std::string body = "Hello, World!";
    resp.set_message_body(body);
    EXPECT_EQ(resp.get_message_body(), body);
    EXPECT_EQ(resp.get_header("Content-Length"), std::to_string(body.size()));
}

TEST_F(HttpResponseTest, ConvertToStringMatchesExample) {
    HttpResponse resp;

    std::string request = 
        "GET /index.html HTTP/1.1\r\n"
        "Host: www.example.com\r\n"
        "User-Agent: curl/7.68.0\r\n"
        "Accept: */*\r\n";

    resp.set_status_code(200);
    resp.set_reason_phrase("OK");
    resp.set_message_body(request);

    resp.set_header("Content-Type", "text/plain");

    std::string expected_response = 
        "HTTP/1.1 200 OK\r\n"
        "Content-Length: 87\r\n"
        "Content-Type: text/plain\r\n"
        "\r\n"
        + request;

    EXPECT_EQ(resp.convert_to_string(), expected_response);
}

TEST_F(HttpResponseTest, ConvertToStringMultipleHeadersNoBody) {
    HttpResponse resp;
    resp.set_status_code(500);
    resp.set_reason_phrase("Internal Server Error");
    resp.set_header("Connection", "close");
    resp.set_header("Content-Type", "text/html");

    std::string expected_response =
        "HTTP/1.1 500 Internal Server Error\r\n"
        "Connection: close\r\n"
        "Content-Length: 0\r\n"
        "Content-Type: text/html\r\n"
        "\r\n";

    EXPECT_EQ(resp.convert_to_string(), expected_response);
}

TEST_F(HttpResponseTest, LargeMessageBody) {
    HttpResponse resp;
    std::string large_body(10000, 'x');
    resp.set_message_body(large_body);
    EXPECT_EQ(resp.get_message_body(), large_body);
    EXPECT_EQ(resp.get_header("Content-Length"), "10000");
}

TEST_F(HttpResponseTest, NonDefaultConstructorBasic) {
    std::map<std::string, std::string> headers = {
        {"Content-Type", "text/plain"}
    };
    
    HttpResponse response("HTTP/1.1", 200, "OK", headers, "Hello World");
    
    EXPECT_EQ(response.get_version(), "HTTP/1.1");
    EXPECT_EQ(response.get_status_code(), 200);
    EXPECT_EQ(response.get_reason_phrase(), "OK");
    EXPECT_EQ(response.get_message_body(), "Hello World");
    EXPECT_EQ(response.get_header("Content-Type"), "text/plain");
}

TEST_F(HttpResponseTest, ContentLengthOverride) {
    std::map<std::string, std::string> headers = {
        {"Content-Type", "text/plain"},
        {"Content-Length", "999"}
    };
    
    std::string body = "Hello";
    HttpResponse response("HTTP/1.1", 200, "OK", headers, body);
    
    EXPECT_EQ(response.get_header("Content-Length"), "5");
    EXPECT_NE(response.get_header("Content-Length"), "999");
}

TEST_F(HttpResponseTest, EmptyBodyConstructor) {
    std::map<std::string, std::string> headers = {
        {"Content-Type", "text/plain"}
    };
    
    HttpResponse response("HTTP/1.1", 204, "No Content", headers, "");
    
    EXPECT_EQ(response.get_message_body(), "");
    EXPECT_EQ(response.get_header("Content-Length"), "0");
}

TEST_F(HttpResponseTest, NotFoundResponse) {
    std::map<std::string, std::string> headers = {
        {"Content-Type", "text/html"}
    };
    
    std::string body = "<h1>404 Not Found</h1>";
    HttpResponse response("HTTP/1.1", 404, "Not Found", headers, body);
    
    EXPECT_EQ(response.get_status_code(), 404);
    EXPECT_EQ(response.get_reason_phrase(), "Not Found");
    EXPECT_EQ(response.get_message_body(), body);
    EXPECT_EQ(response.get_header("Content-Length"), std::to_string(body.size()));
}

TEST_F(HttpResponseTest, MultipleHeadersConstructor) {
    std::map<std::string, std::string> headers = {
        {"Content-Type", "application/json"},
        {"Cache-Control", "no-cache"},
        {"Server", "MyServer/1.0"}
    };
    
    HttpResponse response("HTTP/1.1", 200, "OK", headers, "{\"status\":\"ok\"}");
    
    EXPECT_EQ(response.get_header("Content-Type"), "application/json");
    EXPECT_EQ(response.get_header("Cache-Control"), "no-cache");
    EXPECT_EQ(response.get_header("Server"), "MyServer/1.0");
    EXPECT_EQ(response.get_header("Content-Length"), "15");
}