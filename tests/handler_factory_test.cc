#include "gtest/gtest.h"
#include "handler_factory.h"
#include "echo_handler.h"
#include "file_handler.h"
#include "server_config.h" // For HandlerConfig
#include <memory>
#include <string>
#include <unordered_set>
#include <filesystem>

namespace fs = std::filesystem;

class HandlerFactoryTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create a temporary directory for file handler tests
        test_dir_ = "./handler_factory_test_files";
        fs::create_directories(test_dir_);
    }

    void TearDown() override {
        fs::remove_all(test_dir_);
    }

    HandlerFactory factory;
    std::string test_dir_;
};

// Test creating a valid EchoHandler
TEST_F(HandlerFactoryTest, CreateEchoHandler) {
    HandlerConfig config;
    config.type = "EchoHandler";
    
    auto handler = factory.create_handler(config, "/echo");
    
    ASSERT_NE(handler, nullptr);
    auto* echo_handler = dynamic_cast<EchoHandler*>(handler.get());
    EXPECT_NE(echo_handler, nullptr);
    EXPECT_EQ(handler->get_handler_name(), "EchoHandler");
}

// Test creating a valid FileHandler
TEST_F(HandlerFactoryTest, CreateFileHandler) {
    HandlerConfig config;
    config.type = "StaticHandler";
    config.settings["root"] = test_dir_;
    
    auto handler = factory.create_handler(config, "/static");
    
    ASSERT_NE(handler, nullptr);
    auto* file_handler = dynamic_cast<FileHandler*>(handler.get());
    EXPECT_NE(file_handler, nullptr);
    EXPECT_EQ(handler->get_handler_name(), "FileHandler");
}

// Test creating a FileHandler with specified extensions
TEST_F(HandlerFactoryTest, CreateFileHandlerWithExtensions) {
    HandlerConfig config;
    config.type = "StaticHandler";
    config.settings["root"] = test_dir_;
    config.settings["supported_extensions"] = "html,css";
    
    auto handler = factory.create_handler(config, "/static");
    
    ASSERT_NE(handler, nullptr);
    auto* file_handler = dynamic_cast<FileHandler*>(handler.get());
    EXPECT_NE(file_handler, nullptr);
}

// Test creating a handler with an unknown type
TEST_F(HandlerFactoryTest, CreateUnknownHandler) {
    HandlerConfig config;
    config.type = "non_existent_handler";
    
    auto handler = factory.create_handler(config, "/unknown");
    
    ASSERT_NE(handler, nullptr);  // Should not be nullptr
    EXPECT_EQ(handler->get_handler_name(), "NotFoundHandler");
}

// Test creating a FileHandler without a root setting
TEST_F(HandlerFactoryTest, CreateFileHandlerMissingRootPath) {
    HandlerConfig config;
    config.type = "StaticHandler";
    // No "root" in settings
    
    auto handler = factory.create_handler(config, "/static");
    
    // Expect nullptr because root is required for FileHandler
    EXPECT_EQ(handler, nullptr);
}

// Test parsing a comma-separated extension string
TEST_F(HandlerFactoryTest, ParseExtensions) {
    std::string ext_string = "html, css, js,txt";
    std::unordered_set<std::string> expected = {".html", ".css", ".js", ".txt"};
    
    auto result = factory.parse_extensions(ext_string);
    
    EXPECT_EQ(result, expected);
}

// Test parsing an empty extension string
TEST_F(HandlerFactoryTest, ParseEmptyExtensions) {
    auto result = factory.parse_extensions("");
    EXPECT_TRUE(result.empty());
}

// Test parsing extensions with extra spaces and commas
TEST_F(HandlerFactoryTest, ParseExtensionsWithExtraWhitespaceAndCommas) {
    std::string ext_string = "  .jpg ,.png,  , gif,";
    std::unordered_set<std::string> expected = {".jpg", ".png", ".gif"};
    
    auto result = factory.parse_extensions(ext_string);
    
    EXPECT_EQ(result, expected);
}
