#include "gtest/gtest.h"
#include "path_router.h"
#include "server_config.h"
#include "echo_handler.h"
#include "file_handler.h"
#include "not_found_handler.h"
#include <memory> // Use C++ header <memory> instead of <memory.h>

class PathRouterTest : public ::testing::Test {
protected:
    void SetUp() override {
        routes_.clear();
    }
    
    std::map<std::string, HandlerConfig> routes_;
    
    // Helper function to create a statement
    std::shared_ptr<NginxConfigStatement> CreateStatement(const std::vector<std::string>& tokens) {
        auto stmt = std::make_shared<NginxConfigStatement>();
        stmt->tokens_ = tokens;
        return stmt;
    }

    // Helper to create a mock ServerConfig
    ServerConfig create_config_with_routes(const std::map<std::string, HandlerConfig>& routes) {
        ServerConfig mock_config;
        NginxConfig nginx_config;
        
        // Create the 'server { ... }' block
        auto server_block_stmt = std::make_shared<NginxConfigStatement>();
        server_block_stmt->tokens_.push_back("server");
        server_block_stmt->child_block_ = std::make_unique<NginxConfig>();

        // Add 'listen 8080;' INSIDE the server block
        auto port_statement = std::make_shared<NginxConfigStatement>();
        port_statement->tokens_.push_back("listen");
        port_statement->tokens_.push_back("8080");
        server_block_stmt->child_block_->statements_.push_back(port_statement);
        
        // Add routes INSIDE the server block
        for (const auto& [path, handler_config] : routes) {
            auto location_statement = std::make_shared<NginxConfigStatement>();
            location_statement->tokens_.push_back("location");
            location_statement->tokens_.push_back(path);
            
            auto child_block = std::make_unique<NginxConfig>();
            
            auto handler_statement = std::make_shared<NginxConfigStatement>();
            handler_statement->tokens_.push_back("handler");
            handler_statement->tokens_.push_back(handler_config.type);
            child_block->statements_.push_back(handler_statement);
            
            for (const auto& [key, value] : handler_config.settings) {
                auto setting_statement = std::make_shared<NginxConfigStatement>();
                setting_statement->tokens_.push_back(key);
                setting_statement->tokens_.push_back(value);
                child_block->statements_.push_back(setting_statement);
            }
            
            location_statement->child_block_ = std::move(child_block);
            server_block_stmt->child_block_->statements_.push_back(location_statement);
        }
        
        // Add the server block to the top-level config
        nginx_config.statements_.push_back(server_block_stmt);

        // Load the config
        mock_config.load_from_nginx_config(nginx_config);
        return mock_config;
    }
};

// Test: Match echo handler
TEST_F(PathRouterTest, MatchEchoHandler) {
    HandlerConfig echo_config;
    echo_config.type = "EchoHandler";
    routes_["/echo"] = echo_config;
    
    ServerConfig server_config = create_config_with_routes(routes_);
    PathRouter router(server_config);
    
    auto handler = router.match_handler("/echo");
    ASSERT_NE(handler, nullptr);
    
    auto* echo_handler = dynamic_cast<EchoHandler*>(handler.get());
    EXPECT_NE(echo_handler, nullptr);
}

// Test: Match with prefix path
TEST_F(PathRouterTest, MatchWithPrefixPath) {
    HandlerConfig echo_config;
    echo_config.type = "EchoHandler";
    routes_["/echo"] = echo_config;
    
    ServerConfig server_config = create_config_with_routes(routes_);
    PathRouter router(server_config);
    
    auto handler1 = router.match_handler("/echo");
    EXPECT_NE(handler1, nullptr);
    
    auto handler2 = router.match_handler("/echo/test");
    EXPECT_NE(handler2, nullptr);
    
    auto handler3 = router.match_handler("/echo/test/nested");
    EXPECT_NE(handler3, nullptr);
}

// Test: No match returns nullptr
TEST_F(PathRouterTest, NoMatchReturnsNotFoundHandler) {
    HandlerConfig echo_config;
    echo_config.type = "EchoHandler";
    routes_["/echo"] = echo_config;
    
    ServerConfig server_config = create_config_with_routes(routes_);
    PathRouter router(server_config);
    
    auto handler = router.match_handler("/notfound");
    ASSERT_NE(handler, nullptr);
    EXPECT_EQ(handler->get_handler_name(), "NotFoundHandler");
}

// Test: Multiple routes - order matters with std::map
TEST_F(PathRouterTest, MultipleRoutesOrdering) {
    // std::map orders keys lexicographically, so /api comes before /api/files
    HandlerConfig echo_config;
    echo_config.type = "EchoHandler";
    
    HandlerConfig file_config;
    file_config.type = "file";
    file_config.settings["root"] = "/tmp";
    
    // Test with more specific route first
    routes_["/api/files"] = file_config;
    routes_["/api"] = echo_config;
    
    ServerConfig server_config = create_config_with_routes(routes_);
    PathRouter router(server_config);
    
    // /api/files should match FileHandler if it's checked first
    auto handler = router.match_handler("/api/files/test.txt");
    ASSERT_NE(handler, nullptr);
    
    // Since /api/files is the longest match, we match FileHandler
}

// Test: Exact path match
TEST_F(PathRouterTest, ExactPathMatch) {
    HandlerConfig echo_config;
    echo_config.type = "EchoHandler";
    routes_["/"] = echo_config;
    
    ServerConfig server_config = create_config_with_routes(routes_);
    PathRouter router(server_config);
    
    auto handler = router.match_handler("/");
    EXPECT_NE(handler, nullptr);
}

// Test: Root path matches everything
TEST_F(PathRouterTest, RootPathMatchesEverything) {
    HandlerConfig echo_config;
    echo_config.type = "EchoHandler";
    routes_["/"] = echo_config;
    
    ServerConfig server_config = create_config_with_routes(routes_);
    PathRouter router(server_config);
    
    auto handler1 = router.match_handler("/");
    EXPECT_NE(handler1, nullptr);
    
    auto handler2 = router.match_handler("/anything");
    EXPECT_NE(handler2, nullptr);
    
    auto handler3 = router.match_handler("/anything/nested");
    EXPECT_NE(handler3, nullptr);
}

// Test: Unknown handler type
TEST_F(PathRouterTest, UnknownHandlerType) {
    HandlerConfig unknown_config;
    unknown_config.type = "unknown";
    routes_["/unknown"] = unknown_config;
    
    ServerConfig server_config = create_config_with_routes(routes_);
    PathRouter router(server_config);
    
    auto handler = router.match_handler("/unknown");
    ASSERT_NE(handler, nullptr);
    EXPECT_EQ(handler->get_handler_name(), "NotFoundHandler");

    HttpResponse response = handler->handle_request(HttpRequest{});
    EXPECT_EQ(response.get_status_code(), 404);
    EXPECT_EQ(response.get_reason_phrase(), "Not Found");
    EXPECT_EQ(response.get_message_body(), "<h1>404 Not Found</h1>");
}

// Test: Empty routes - should fail to load config
TEST_F(PathRouterTest, EmptyRoutes) {
    NginxConfig nginx_config;
    
    // Create 'server { listen 8080; }'
    auto server_block_stmt = std::make_shared<NginxConfigStatement>();
    server_block_stmt->tokens_.push_back("server");
    server_block_stmt->child_block_ = std::make_unique<NginxConfig>();

    auto port_statement = std::make_shared<NginxConfigStatement>();
    port_statement->tokens_.push_back("listen");
    port_statement->tokens_.push_back("8080");
    server_block_stmt->child_block_->statements_.push_back(port_statement);

    nginx_config.statements_.push_back(server_block_stmt);

    ServerConfig server_config;
    // This should return false because routes are empty
    EXPECT_FALSE(server_config.load_from_nginx_config(nginx_config));
}

// Test: Case sensitivity
TEST_F(PathRouterTest, CaseSensitivity) {
    HandlerConfig echo_config;
    echo_config.type = "EchoHandler";
    routes_["/Echo"] = echo_config;
    
    ServerConfig server_config = create_config_with_routes(routes_);
    PathRouter router(server_config);
    
    auto handler1 = router.match_handler("/Echo");
    EXPECT_NE(handler1, nullptr);
    
    auto handler2 = router.match_handler("/echo");
    ASSERT_NE(handler2, nullptr);
    EXPECT_EQ(handler2->get_handler_name(), "NotFoundHandler"); // Should not match - case sensitive
}

// Test: Trailing slash handling
TEST_F(PathRouterTest, TrailingSlash) {
    HandlerConfig echo_config;
    echo_config.type = "EchoHandler";
    routes_["/echo"] = echo_config;
    
    ServerConfig server_config = create_config_with_routes(routes_);
    PathRouter router(server_config);
    
    auto handler1 = router.match_handler("/echo");
    EXPECT_NE(handler1, nullptr);
    
    auto handler2 = router.match_handler("/echo/");
    EXPECT_NE(handler2, nullptr);  // Should match - prefix matching
}

TEST_F(PathRouterTest, FileHandlerWithSupportedExtensions) {
    HandlerConfig file_config;
    file_config.type = "file";
    file_config.settings["root"] = "/";
    file_config.settings["supported_extensions"] = "html,txt,css";

    routes_["/static"] = file_config;

    ServerConfig server_config = create_config_with_routes(routes_);
    PathRouter router(server_config);

    auto handler1 = router.match_handler("/static/index.html");
    EXPECT_NE(handler1, nullptr);

    auto handler2 = router.match_handler("/static/style.css");
    EXPECT_NE(handler2, nullptr);
}

TEST_F(PathRouterTest, FileHandlerWithoutExtensions) {
    HandlerConfig file_config;
    file_config.type = "file";
    file_config.settings["root"] = "/";

    routes_["/static"] = file_config;

    ServerConfig server_config = create_config_with_routes(routes_);
    PathRouter router(server_config);

    auto handler1 = router.match_handler("/static/test.txt");
    EXPECT_NE(handler1, nullptr);

    auto handler2 = router.match_handler("/static/any/other/path");
    EXPECT_NE(handler2, nullptr);
}

// Test: Unknown paths should be routed to NotFoundHandler
TEST_F(PathRouterTest, UnknownPathGoesToNotFoundHandler) {
    HandlerConfig not_found_config;
    not_found_config.type = "NotFoundHandler";
    routes_["/"] = not_found_config;

    ServerConfig server_config = create_config_with_routes(routes_);
    PathRouter router(server_config);

    HttpRequest request;
    request.set_path("/some/unknown/path");

    auto handler = router.match_handler(request.path());
    ASSERT_NE(handler, nullptr);
    EXPECT_EQ(handler->get_handler_name(), "NotFoundHandler");

    HttpResponse response = handler->handle_request(request);
    EXPECT_EQ(response.get_status_code(), 404);
    EXPECT_EQ(response.get_reason_phrase(), "Not Found");
    EXPECT_EQ(response.get_message_body(), "<h1>404 Not Found</h1>");
}

// Test: Empty path should also return NotFoundHandler
TEST_F(PathRouterTest, EmptyPathReturnsNotFoundHandler) {
    ServerConfig server_config = create_config_with_routes(routes_);
    PathRouter router(server_config);

    HttpRequest request;
    request.set_path("");

    auto handler = router.match_handler(request.path());
    ASSERT_NE(handler, nullptr);
    EXPECT_EQ(handler->get_handler_name(), "NotFoundHandler");

    HttpResponse response = handler->handle_request(request);
    EXPECT_EQ(response.get_status_code(), 404);
    EXPECT_EQ(response.get_reason_phrase(), "Not Found");
    EXPECT_EQ(response.get_message_body(), "<h1>404 Not Found</h1>");
}

// Test: Deep nested paths return NotFoundHandler
TEST_F(PathRouterTest, DeepNestedPathReturnsNotFoundHandler) {
    HandlerConfig not_found_config;
    not_found_config.type = "NotFoundHandler";
    routes_["/"] = not_found_config;

    ServerConfig server_config = create_config_with_routes(routes_);
    PathRouter router(server_config);

    HttpRequest request;
    request.set_path("/a/b/c/d/e/f/g");

    auto handler = router.match_handler(request.path());
    ASSERT_NE(handler, nullptr);
    EXPECT_EQ(handler->get_handler_name(), "NotFoundHandler");

    HttpResponse response = handler->handle_request(request);
    EXPECT_EQ(response.get_status_code(), 404);
    EXPECT_EQ(response.get_reason_phrase(), "Not Found");
    EXPECT_EQ(response.get_message_body(), "<h1>404 Not Found</h1>");
}

