#include "gtest/gtest.h"
#include "config_parser.h"
#include <sstream>
#include <fstream>
#include <cstdio>

class ParserTest : public testing::Test {
  protected:
    NginxConfigParser parser;
    NginxConfig out_config;
};

TEST_F(ParserTest, EmptyConfig) {
  std::stringstream config("");
  bool success = parser.Parse(&config, &out_config);
  EXPECT_TRUE(success);
}

TEST_F(ParserTest, NestedBlock) {
  std::stringstream config("server {\n  listen   80;\n  server {\n    listen 20;\n  }\n}");
  bool success = parser.Parse(&config, &out_config);
  EXPECT_TRUE(success);
}

TEST_F(ParserTest, EmptyBlock) {
  std::stringstream config("server { }");
  bool success = parser.Parse(&config, &out_config);
  EXPECT_TRUE(success);
}

TEST_F(ParserTest, OpenBlock) {
  std::stringstream config("server {\n  listen   80;\n  server {\n    listen 20;\n  \n}");
  bool success = parser.Parse(&config, &out_config);
  EXPECT_FALSE(success);
}

TEST_F(ParserTest, InlineQuote) {
  std::stringstream config("server abc\"def;");
  bool success = parser.Parse(&config, &out_config);
  EXPECT_FALSE(success);
}

TEST_F(ParserTest, SingleQuote) {
  std::stringstream config("server { listen '80'; }");
  bool success = parser.Parse(&config, &out_config);
  EXPECT_TRUE(success);
}

TEST_F(ParserTest, DoubleQuote) {
  std::stringstream config("server { listen \"80\"; }");
  bool success = parser.Parse(&config, &out_config);
  EXPECT_TRUE(success);
}

TEST_F(ParserTest, UnclosedSingleQuote) {
  std::stringstream config("server { listen '80; }");
  bool success = parser.Parse(&config, &out_config);
  EXPECT_FALSE(success);
}

TEST_F(ParserTest, UnclosedDoubleQuote) {
  std::stringstream config("server { listen \"80; }");
  bool success = parser.Parse(&config, &out_config);
  EXPECT_FALSE(success);
}

TEST_F(ParserTest, NoSemicolon) {
  std::stringstream config("server { listen 80 }");
  bool success = parser.Parse(&config, &out_config);
  EXPECT_FALSE(success);
}

TEST_F(ParserTest, MultipleStatements) {
  std::stringstream config("server { listen 80; } web_server { listen 8080; }");
  bool success = parser.Parse(&config, &out_config);
  EXPECT_TRUE(success);
}

TEST_F(ParserTest, Whitespace) {
  std::stringstream config("  server  {  listen  80  ;}");
  bool success = parser.Parse(&config, &out_config);
  EXPECT_TRUE(success);
}

TEST_F(ParserTest, OpenBlockSemicolon) {
  std::stringstream config("server { listen 80;");
  bool success = parser.Parse(&config, &out_config);
  EXPECT_FALSE(success);
}

// Test ToString method
TEST_F(ParserTest, ToStringEmptyConfig) {
  std::stringstream config("");
  parser.Parse(&config, &out_config);
  std::string result = out_config.ToString();
  EXPECT_EQ(result, "");
}

TEST_F(ParserTest, ToStringNestedConfig) {
  std::stringstream config("server {\n  listen   80;\n  server {\n    listen 20;\n  }\n}");
  parser.Parse(&config, &out_config);
  std::string result = out_config.ToString();
  EXPECT_TRUE(result.find("server") != std::string::npos);
  EXPECT_TRUE(result.find("listen") != std::string::npos);
}

TEST_F(ParserTest, ExtraEndBlock) {
  std::stringstream config("server { listen 80; } }");
  bool success = parser.Parse(&config, &out_config);
  EXPECT_FALSE(success);
}

TEST_F(ParserTest, NoContextBlock) {
  std::stringstream config("{ }");
  bool success = parser.Parse(&config, &out_config);
  EXPECT_FALSE(success);
}

TEST_F(ParserTest, SemicolonInBlockOnly) {
  std::stringstream config("server { ; }");
  bool success = parser.Parse(&config, &out_config);
  EXPECT_FALSE(success);
}

TEST_F(ParserTest, Comments) {
  std::stringstream config("# CS130 is so fun \nserver { listen 80; }");
  bool success = parser.Parse(&config, &out_config);
  EXPECT_TRUE(success);
}

// Tests for file-parsing variant of parser
TEST_F(ParserTest, FileParse_ValidFile) {
  const char* filename = "temp_valid_config.conf";
  {
    std::ofstream ofs(filename);
    ofs << "server { listen 80; }";
  }
  bool success = parser.Parse(filename, &out_config);
  EXPECT_TRUE(success);
  std::remove(filename);
}

TEST_F(ParserTest, FileParse_MissingFile) {
  const char* filename = "nonexistent_file.conf";
  bool success = parser.Parse(filename, &out_config);
  EXPECT_FALSE(success);
}

TEST_F(ParserTest, FileParse_EmptyFile) {
  const char* filename = "temp_empty_config.conf";
  {
    std::ofstream ofs(filename);
  }
  bool success = parser.Parse(filename, &out_config);
  EXPECT_TRUE(success);
  std::remove(filename);
}