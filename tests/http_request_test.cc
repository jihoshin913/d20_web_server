// HttpRequest_test.cpp
#include "http_request.h"
#include <gtest/gtest.h>

class HttpRequestTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Common setup if needed
    }
};

// Test basic GET request parsing
TEST_F(HttpRequestTest, ParseBasicGetRequest) {
    std::string raw = 
        "GET /index.html HTTP/1.1\r\n"
        "Host: localhost:8080\r\n"
        "User-Agent: Mozilla/5.0\r\n"
        "\r\n";
    
    HttpRequest req = HttpRequest::parse(raw);
    
    EXPECT_EQ(req.method(), "GET");
    EXPECT_EQ(req.path(), "/index.html");
    EXPECT_EQ(req.version(), "HTTP/1.1");
    EXPECT_TRUE(req.is_valid());
}

// Test parsing headers
TEST_F(HttpRequestTest, ParseHeaders) {
    std::string raw = 
        "GET /test HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "Content-Type: text/html\r\n"
        "Accept: */*\r\n"
        "\r\n";
    
    HttpRequest req = HttpRequest::parse(raw);
    
    EXPECT_TRUE(req.has_header("Host"));
    EXPECT_TRUE(req.has_header("Content-Type"));
    EXPECT_TRUE(req.has_header("Accept"));
    
    EXPECT_EQ(req.get_header("Host").value(), "localhost");
    EXPECT_EQ(req.get_header("Content-Type").value(), "text/html");
    EXPECT_EQ(req.get_header("Accept").value(), "*/*");
}

// Test case-insensitive header lookup
TEST_F(HttpRequestTest, CaseInsensitiveHeaders) {
    std::string raw = 
        "GET / HTTP/1.1\r\n"
        "Content-Type: application/json\r\n"
        "\r\n";
    
    HttpRequest req = HttpRequest::parse(raw);
    
    // All of these should work
    EXPECT_TRUE(req.has_header("Content-Type"));
    EXPECT_TRUE(req.has_header("content-type"));
    EXPECT_TRUE(req.has_header("CONTENT-TYPE"));
    EXPECT_TRUE(req.has_header("CoNtEnT-TyPe"));
    
    EXPECT_EQ(req.get_header("content-type").value(), "application/json");
    EXPECT_EQ(req.get_header("CONTENT-TYPE").value(), "application/json");
}

// Test header that doesn't exist
TEST_F(HttpRequestTest, NonExistentHeader) {
    std::string raw = 
        "GET / HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "\r\n";
    
    HttpRequest req = HttpRequest::parse(raw);
    
    EXPECT_FALSE(req.has_header("Authorization"));
    EXPECT_FALSE(req.get_header("Authorization").has_value());
}

// Test parsing with body
TEST_F(HttpRequestTest, ParseRequestWithBody) {
    std::string raw = 
        "POST /api/users HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "Content-Type: application/json\r\n"
        "Content-Length: 27\r\n"
        "\r\n"
        "{\"name\":\"John\",\"age\":30}";
    
    HttpRequest req = HttpRequest::parse(raw);
    
    EXPECT_EQ(req.method(), "POST");
    EXPECT_EQ(req.path(), "/api/users");
    EXPECT_EQ(req.body(), "{\"name\":\"John\",\"age\":30}");
}

// Test multi-line body
TEST_F(HttpRequestTest, ParseMultiLineBody) {
    std::string raw = 
        "POST /api/data HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "\r\n"
        "Line 1\n"
        "Line 2\n"
        "Line 3";
    
    HttpRequest req = HttpRequest::parse(raw);
    
    EXPECT_EQ(req.body(), "Line 1\nLine 2\nLine 3");
}

// Test empty body
TEST_F(HttpRequestTest, EmptyBody) {
    std::string raw = 
        "GET /index.html HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "\r\n";
    
    HttpRequest req = HttpRequest::parse(raw);
    
    EXPECT_EQ(req.body(), "");
    EXPECT_TRUE(req.body().empty());
}

// Test invalid request (malformed request line)
TEST_F(HttpRequestTest, InvalidRequestLine) {
    std::string raw = "INVALID\r\n\r\n";
    
    HttpRequest req = HttpRequest::parse(raw);
    
    EXPECT_FALSE(req.is_valid());
    EXPECT_TRUE(req.method().empty());
    EXPECT_TRUE(req.path().empty());
}

// Test incomplete request line
TEST_F(HttpRequestTest, IncompleteRequestLine) {
    std::string raw = "GET /path\r\n\r\n";  // Missing version
    
    HttpRequest req = HttpRequest::parse(raw);
    
    EXPECT_FALSE(req.is_valid());
    EXPECT_TRUE(req.method().empty());
    EXPECT_TRUE(req.path().empty());
}

// Test empty method
TEST_F(HttpRequestTest, EmptyMethod) {
    HttpRequest req;
    req.set_path("/test");
    req.set_version("HTTP/1.1");
    req.set_body("");
    // method is empty by default
    
    EXPECT_FALSE(req.is_valid());
}

// Test empty path
TEST_F(HttpRequestTest, EmptyPath) {
    HttpRequest req;
    req.set_method("GET");
    req.set_version("HTTP/1.1");
    // path is empty by default
    
    EXPECT_FALSE(req.is_valid());
}

// Test empty version
TEST_F(HttpRequestTest, EmptyVersion) {
    HttpRequest req;
    req.set_method("GET");
    req.set_path("/test");
    // version is empty by default
    
    EXPECT_FALSE(req.is_valid());
}

// Test all fields empty
TEST_F(HttpRequestTest, AllFieldsEmpty) {
    HttpRequest req;
    
    EXPECT_FALSE(req.is_valid());
}

// Test valid HTTP/1.1 version
TEST_F(HttpRequestTest, ValidHTTP11Version) {
    std::string raw = "GET /test HTTP/1.1\r\n\r\n";
    HttpRequest req = HttpRequest::parse(raw);
    
    EXPECT_TRUE(req.is_valid());
    EXPECT_EQ(req.version(), "HTTP/1.1");
}

// Test invalid HTTP/1.0 version
TEST_F(HttpRequestTest, InvalidHTTP10Version) {
    std::string raw = "GET /test HTTP/1.0\r\n\r\n";
    HttpRequest req = HttpRequest::parse(raw);
    
    EXPECT_FALSE(req.is_valid());
    EXPECT_EQ(req.version(), "HTTP/1.0");
}

// Test completely wrong version format
TEST_F(HttpRequestTest, WrongVersionFormat) {
    std::string raw = "GET /test HTTPS\r\n\r\n";
    HttpRequest req = HttpRequest::parse(raw);
    
    EXPECT_FALSE(req.is_valid());
}

// Test lowercase http version
TEST_F(HttpRequestTest, LowercaseHTTPVersion) {
    std::string raw = "GET /test http/1.1\r\n\r\n";
    HttpRequest req = HttpRequest::parse(raw);
    
    EXPECT_FALSE(req.is_valid());
}

// Test valid GET method
TEST_F(HttpRequestTest, ValidGETMethod) {
    std::string raw = "GET /test HTTP/1.1\r\n\r\n";
    HttpRequest req = HttpRequest::parse(raw);
    
    EXPECT_TRUE(req.is_valid());
    EXPECT_EQ(req.method(), "GET");
}

// Test valid POST method
TEST_F(HttpRequestTest, ValidPOSTMethod) {
    std::string raw = "POST /test HTTP/1.1\r\n\r\n";
    HttpRequest req = HttpRequest::parse(raw);
    
    EXPECT_TRUE(req.is_valid());
    EXPECT_EQ(req.method(), "POST");
}

// Test valid PUT method
TEST_F(HttpRequestTest, ValidPUTMethod) {
    std::string raw = "PUT /test HTTP/1.1\r\n\r\n";
    HttpRequest req = HttpRequest::parse(raw);
    
    EXPECT_TRUE(req.is_valid());
    EXPECT_EQ(req.method(), "PUT");
}

// Test valid DELETE method
TEST_F(HttpRequestTest, ValidDELETEMethod) {
    std::string raw = "DELETE /test HTTP/1.1\r\n\r\n";
    HttpRequest req = HttpRequest::parse(raw);
    
    EXPECT_TRUE(req.is_valid());
    EXPECT_EQ(req.method(), "DELETE");
}

// Test invalid HEAD method
TEST_F(HttpRequestTest, InvalidHEADMethod) {
    std::string raw = "HEAD /test HTTP/1.1\r\n\r\n";
    HttpRequest req = HttpRequest::parse(raw);
    
    EXPECT_FALSE(req.is_valid());
    EXPECT_EQ(req.method(), "HEAD");
}

// Test lowercase method
TEST_F(HttpRequestTest, LowercaseMethod) {
    std::string raw = "get /test HTTP/1.1\r\n\r\n";
    HttpRequest req = HttpRequest::parse(raw);
    
    EXPECT_FALSE(req.is_valid());
    EXPECT_EQ(req.method(), "get");
}


// Test valid path starting with /
TEST_F(HttpRequestTest, ValidPathStartsWithSlash) {
    std::string raw = "GET /test HTTP/1.1\r\n\r\n";
    HttpRequest req = HttpRequest::parse(raw);
    
    EXPECT_TRUE(req.is_valid());
    EXPECT_EQ(req.path(), "/test");
}

// Test valid root path
TEST_F(HttpRequestTest, ValidRootPath) {
    std::string raw = "GET / HTTP/1.1\r\n\r\n";
    HttpRequest req = HttpRequest::parse(raw);
    
    EXPECT_TRUE(req.is_valid());
    EXPECT_EQ(req.path(), "/");
}

// Test invalid path not starting with /
TEST_F(HttpRequestTest, PathDoesNotStartWithSlash) {
    std::string raw = "GET test HTTP/1.1\r\n\r\n";
    HttpRequest req = HttpRequest::parse(raw);
    
    EXPECT_FALSE(req.is_valid());
    EXPECT_EQ(req.path(), "test");
}

// Test path starting with letter
TEST_F(HttpRequestTest, PathStartsWithLetter) {
    std::string raw = "GET api/test HTTP/1.1\r\n\r\n";
    HttpRequest req = HttpRequest::parse(raw);
    
    EXPECT_FALSE(req.is_valid());
}

// Test path with space in middle
TEST_F(HttpRequestTest, PathWithSpaceInMiddle) {
    std::string raw = "GET /test path HTTP/1.1\r\n\r\n";
    HttpRequest req = HttpRequest::parse(raw);
    
    EXPECT_FALSE(req.is_valid());
}

// Test path with space at beginning (after /)
TEST_F(HttpRequestTest, PathWithSpaceAtBeginning) {
    std::string raw = "GET / test HTTP/1.1\r\n\r\n";
    HttpRequest req = HttpRequest::parse(raw);
    
    EXPECT_FALSE(req.is_valid());
}

// Test valid request with proper header termination
TEST_F(HttpRequestTest, ValidHeaderTermination) {
    std::string raw = "GET /test HTTP/1.1\r\nHost: localhost\r\n\r\n";
    HttpRequest req = HttpRequest::parse(raw);
    
    EXPECT_TRUE(req.is_valid());
}

// Test request missing \r\n\r\n termination
TEST_F(HttpRequestTest, MissingHeaderTermination) {
    std::string raw = "GET /test HTTP/1.1\r\nHost: localhost";
    HttpRequest req = HttpRequest::parse(raw);
    
    EXPECT_FALSE(req.is_valid());
}

// Test request with \r\n but no double
TEST_F(HttpRequestTest, SingleCRLF) {
    std::string raw = "GET /test HTTP/1.1\r\n";
    HttpRequest req = HttpRequest::parse(raw);
    
    EXPECT_FALSE(req.is_valid());
}

// Test request with proper termination and body
TEST_F(HttpRequestTest, ValidHeaderTerminationWithBody) {
    std::string raw = "POST /api HTTP/1.1\r\nContent-Type: application/json\r\n\r\n{\"key\":\"value\"}";
    HttpRequest req = HttpRequest::parse(raw);
    
    EXPECT_TRUE(req.is_valid());
    EXPECT_EQ(req.body(), "{\"key\":\"value\"}");
}

// Test empty string
TEST_F(HttpRequestTest, EmptyString) {
    std::string raw = "";
    HttpRequest req = HttpRequest::parse(raw);
    
    EXPECT_FALSE(req.is_valid());
}

// Test valid complex request
TEST_F(HttpRequestTest, ValidComplexRequest) {
    std::string raw = "POST /api/users/123 HTTP/1.1\r\n"
                      "Host: example.com\r\n"
                      "Content-Type: application/json\r\n"
                      "Content-Length: 27\r\n"
                      "\r\n"
                      "{\"name\":\"John\",\"age\":30}";
    HttpRequest req = HttpRequest::parse(raw);
    
    EXPECT_TRUE(req.is_valid());
    EXPECT_EQ(req.method(), "POST");
    EXPECT_EQ(req.path(), "/api/users/123");
    EXPECT_EQ(req.version(), "HTTP/1.1");
}

// Test add_header method
TEST_F(HttpRequestTest, AddSingleHeader) {
    HttpRequest req;
    req.add_header("Content-Type", "text/plain");

    EXPECT_TRUE(req.has_header("Content-Type"));
    EXPECT_EQ(req.get_header("Content-Type").value(), "text/plain");
}

// Overwrite the header
TEST_F(HttpRequestTest, OverwriteExistingHeader) {
    HttpRequest req;
    req.add_header("Host", "localhost");
    req.add_header("Host", "127.0.0.1");

    EXPECT_TRUE(req.has_header("Host"));
    EXPECT_EQ(req.get_header("Host").value(), "127.0.0.1");
}

// Basic request with no headers or body
TEST_F(HttpRequestTest, ToStringBasicRequest) {
    HttpRequest req;
    req.set_method("GET");
    req.set_path("/index.html");
    req.set_version("HTTP/1.1");

    std::string str = req.to_string();

    EXPECT_NE(str.find("Method: GET"), std::string::npos);
    EXPECT_NE(str.find("Path: /index.html"), std::string::npos);
    EXPECT_NE(str.find("Version: HTTP/1.1"), std::string::npos);

    EXPECT_EQ(str.find("Headers:"), std::string::npos);
    EXPECT_EQ(str.find("Body:"), std::string::npos);
}

// Request with headers
TEST_F(HttpRequestTest, ToStringWithHeaders) {
    HttpRequest req;
    req.set_method("POST");
    req.set_path("/api");
    req.set_version("HTTP/1.1");
    req.add_header("Host", "localhost");
    req.add_header("Content-Type", "application/json");

    std::string str = req.to_string();

    EXPECT_NE(str.find("Method: POST"), std::string::npos);
    EXPECT_NE(str.find("Headers:"), std::string::npos);
    EXPECT_NE(str.find("Host: localhost"), std::string::npos);
    EXPECT_NE(str.find("Content-Type: application/json"), std::string::npos);
}

// Request with headers and body
TEST_F(HttpRequestTest, ToStringWithHeadersAndBody) {
    HttpRequest req;
    req.set_method("PUT");
    req.set_path("/user/5");
    req.set_version("HTTP/1.0");
    req.add_header("Content-Length", "15");
    req.add_header("Accept", "application/json");
    req.set_body("{\"name\":\"Bob\"}");

    std::string str = req.to_string();

    EXPECT_NE(str.find("Method: PUT"), std::string::npos);
    EXPECT_NE(str.find("Path: /user/5"), std::string::npos);
    EXPECT_NE(str.find("Version: HTTP/1.0"), std::string::npos);
    EXPECT_NE(str.find("Headers:"), std::string::npos);
    EXPECT_NE(str.find("Content-Length: 15"), std::string::npos);
    EXPECT_NE(str.find("Accept: application/json"), std::string::npos);
    EXPECT_NE(str.find("Body:"), std::string::npos);
    EXPECT_NE(str.find("{\"name\":\"Bob\"}"), std::string::npos);
}

// Test an all whitespace string
TEST_F(HttpRequestTest, ParseHeaderWithOnlyWhitespaceValue) {
    std::string raw =
        "GET / HTTP/1.1\r\n"
        "X-Empty:   \r\n"  // header value = only whitespace
        "\r\n";
    
    HttpRequest req = HttpRequest::parse(raw);

    EXPECT_TRUE(req.has_header("X-Empty"));
    EXPECT_EQ(req.get_header("X-Empty").value(), "");
}

// ============================================================================
// QUERY PARAMETER TESTS
// ============================================================================

// Test parsing path with query parameters
TEST_F(HttpRequestTest, ParsePathWithQueryParameters) {
    std::string raw = "GET /api/file_data?name=test&tag=d20 HTTP/1.1\r\n\r\n";
    HttpRequest req = HttpRequest::parse(raw);
    
    EXPECT_TRUE(req.is_valid());
    EXPECT_EQ(req.path(), "/api/file_data?name=test&tag=d20");
    EXPECT_EQ(req.base_path(), "/api/file_data");
}

// Test get_query_param with existing parameter
TEST_F(HttpRequestTest, GetQueryParamExists) {
    HttpRequest req;
    req.set_path("/api/file_data?name=test&tag=d20");
    
    auto name = req.get_query_param("name");
    ASSERT_TRUE(name.has_value());
    EXPECT_EQ(name.value(), "test");
    
    auto tag = req.get_query_param("tag");
    ASSERT_TRUE(tag.has_value());
    EXPECT_EQ(tag.value(), "d20");
}

// Test get_query_param with non-existent parameter
TEST_F(HttpRequestTest, GetQueryParamDoesNotExist) {
    HttpRequest req;
    req.set_path("/api/file_data?name=test");
    
    auto missing = req.get_query_param("tag");
    EXPECT_FALSE(missing.has_value());
}

// Test path without query parameters
TEST_F(HttpRequestTest, PathWithoutQueryParameters) {
    HttpRequest req;
    req.set_path("/api/file_data");
    
    EXPECT_EQ(req.path(), "/api/file_data");
    EXPECT_EQ(req.base_path(), "/api/file_data");
    EXPECT_TRUE(req.get_query_params().empty());
}

// Test get_query_params returns all parameters
TEST_F(HttpRequestTest, GetAllQueryParams) {
    HttpRequest req;
    req.set_path("/api/data?name=test&tag=d20&id=123");
    
    auto params = req.get_query_params();
    EXPECT_EQ(params.size(), 3);
    EXPECT_EQ(params["name"], "test");
    EXPECT_EQ(params["tag"], "d20");
    EXPECT_EQ(params["id"], "123");
}

// Test single query parameter
TEST_F(HttpRequestTest, SingleQueryParameter) {
    HttpRequest req;
    req.set_path("/search?q=hello");
    
    EXPECT_EQ(req.base_path(), "/search");
    
    auto q = req.get_query_param("q");
    ASSERT_TRUE(q.has_value());
    EXPECT_EQ(q.value(), "hello");
}

// Test empty query string (just ?)
TEST_F(HttpRequestTest, EmptyQueryString) {
    HttpRequest req;
    req.set_path("/api/data?");
    
    EXPECT_EQ(req.base_path(), "/api/data");
    EXPECT_TRUE(req.get_query_params().empty());
}

// Test query parameter with empty value
TEST_F(HttpRequestTest, QueryParameterWithEmptyValue) {
    HttpRequest req;
    req.set_path("/api/data?name=&tag=d20");
    
    auto name = req.get_query_param("name");
    ASSERT_TRUE(name.has_value());
    EXPECT_EQ(name.value(), "");
    
    auto tag = req.get_query_param("tag");
    ASSERT_TRUE(tag.has_value());
    EXPECT_EQ(tag.value(), "d20");
}

// Test query parameter without equals sign (malformed)
TEST_F(HttpRequestTest, QueryParameterWithoutEquals) {
    HttpRequest req;
    req.set_path("/api/data?invalid&name=test");
    
    auto invalid = req.get_query_param("invalid");
    EXPECT_FALSE(invalid.has_value());
    
    auto name = req.get_query_param("name");
    ASSERT_TRUE(name.has_value());
    EXPECT_EQ(name.value(), "test");
}

// Test query parameters with special characters in values
TEST_F(HttpRequestTest, QueryParametersWithSpecialCharacters) {
    HttpRequest req;
    req.set_path("/api/data?name=hello-world&tag=d20_project");
    
    auto name = req.get_query_param("name");
    ASSERT_TRUE(name.has_value());
    EXPECT_EQ(name.value(), "hello-world");
    
    auto tag = req.get_query_param("tag");
    ASSERT_TRUE(tag.has_value());
    EXPECT_EQ(tag.value(), "d20_project");
}

// Test parse() with query parameters
TEST_F(HttpRequestTest, ParseWithQueryParameters) {
    std::string raw = 
        "GET /api/file_data?name=document&tag=test HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "\r\n";
    
    HttpRequest req = HttpRequest::parse(raw);
    
    EXPECT_TRUE(req.is_valid());
    EXPECT_EQ(req.base_path(), "/api/file_data");
    
    auto name = req.get_query_param("name");
    ASSERT_TRUE(name.has_value());
    EXPECT_EQ(name.value(), "document");
    
    auto tag = req.get_query_param("tag");
    ASSERT_TRUE(tag.has_value());
    EXPECT_EQ(tag.value(), "test");
}
