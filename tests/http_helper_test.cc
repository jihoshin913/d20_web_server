#include "http_helper.h"
#include "gtest/gtest.h"
#include <string>

class HttpDetectTest : public testing::Test {};

TEST_F(HttpDetectTest, DetectRequstLine) {
  std::string buffer = "GET /index.html HTTP/1.1\r\n\r\n";
  EXPECT_TRUE(detect_http_request(buffer));
}

TEST_F(HttpDetectTest, DetectsHostLine) {
  std::string buffer = "GET /index.html HTTP/1.1\r\nHost: www.example.com\r\n\r\n";
  EXPECT_TRUE(detect_http_request(buffer));
}

TEST_F(HttpDetectTest, IncompleteRequest) {
  std::string buffer = "GET /index.html HTTP/1.1\r\nHost: www.example.com\r\n";
  EXPECT_FALSE(detect_http_request(buffer));
}

TEST_F(HttpDetectTest, EmptyBuffer) {
  std::string buffer = "";
  EXPECT_FALSE(detect_http_request(buffer));
}

TEST_F(HttpDetectTest, MultipleRequests) {
  std::string buffer = "GET /index.html HTTP/1.1\r\n\r\nGET /index.html HTTP/1.1\r\nHost: www.example.com\r\n\r\n";
  EXPECT_TRUE(detect_http_request(buffer));
}

TEST_F(HttpDetectTest, NewlinesOnly) {
  std::string buffer = "\n\n";
  EXPECT_TRUE(detect_http_request(buffer));
}

TEST_F(HttpDetectTest, NewlinesWithCarrigeReturns) {
  std::string buffer = "\r\n\r\n";
  EXPECT_TRUE(detect_http_request(buffer));
}

TEST(HttpHelperTest, CheckMalformedRequestEmpty) {
    std::string empty_buffer = "";
    EXPECT_EQ(check_malformed_request(empty_buffer), MalformedType::Empty);
}

TEST(HttpHelperTest, CheckMalformedRequestNoHeaderTerminator) {
    std::string no_terminator = "GET /path HTTP/1.1";
    EXPECT_EQ(check_malformed_request(no_terminator), MalformedType::NoHeaderTerminator);
}

TEST(HttpHelperTest, CheckMalformedRequestValid) {
    std::string valid_request = "GET /path HTTP/1.1\r\nHost: example.com\r\n\r\n";
    EXPECT_EQ(check_malformed_request(valid_request), MalformedType::None);
}

TEST(HttpHelperTest, CheckMalformedRequestOnlyCRLF) {
    std::string only_crlf = "\r\n\r\n";
    EXPECT_EQ(check_malformed_request(only_crlf), MalformedType::None);
}

TEST(HttpHelperTest, CheckMalformedRequestRandomText) {
    std::string garbage = "GARBAGE DATA RANDOM TEXT";
    EXPECT_EQ(check_malformed_request(garbage), MalformedType::NoHeaderTerminator);
}
