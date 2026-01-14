// server_config.cc
#include "server_config.h"
#include <iostream>

// Disclaimer: This functionality is written by our group, and wrapped in the server config object with the help with Claude Sonnet 4.5. (Tony)
bool ServerConfig::load_from_nginx_config(const NginxConfig& config) {
    // Iterate through top-level statements (e.g., 'server { ... }')
    for (const auto& statement : config.statements_) {
        
        if (statement->tokens_[0] == "server" && statement->child_block_) {
            
            const NginxConfig& server_block = *statement->child_block_;
            for (const auto& server_statement : server_block.statements_) {
                
                // Parse port
                if (server_statement->tokens_[0] == "listen" && server_statement->tokens_.size() >= 2) {
                    try {
                        port_ = std::stoi(server_statement->tokens_[1]);
                    } catch (...) {
                        std::cerr << "Invalid port number\n";
                        return false;
                    }
                }
                
                // Parse location blocks
                if (server_statement->tokens_[0] == "location" && server_statement->tokens_.size() >= 2) {
                    std::string path = server_statement->tokens_[1];
                    
                    if (server_statement->child_block_) {
                        HandlerConfig handler_config = parse_handler_config(*server_statement->child_block_);
                        routes_[path] = handler_config;
                    }
                }
            }

            return port_ > 0 && !routes_.empty();
        }
    }
    
    std::cerr << "No valid 'server' block found in config.\n";
    return false;
}
HandlerConfig ServerConfig::parse_handler_config(const NginxConfig& block) {
    HandlerConfig config;
    
    for (const auto& statement : block.statements_) {
        if (statement->tokens_.empty()) continue;
        
        const std::string& key = statement->tokens_[0];
        
        // Parse handler type
        if (key == "handler" && statement->tokens_.size() >= 2) {
            config.type = statement->tokens_[1];
        }
        // Parse handler settings (everything else)
        else if (statement->tokens_.size() >= 2) {
            config.settings[key] = statement->tokens_[1];
        }
    }
    
    return config;
}