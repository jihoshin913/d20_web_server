// server_config.h
#ifndef SERVER_CONFIG_H
#define SERVER_CONFIG_H

#include <string>
#include <map>
#include "config_parser.h"

struct HandlerConfig {
    std::string type;  // "echo", "file", "proxy", etc.
    std::map<std::string, std::string> settings;  // Handler-specific settings
};

class ServerConfig {
public:
    ServerConfig() = default;
    
    // Load from parsed nginx config
    bool load_from_nginx_config(const NginxConfig& config);
    
    // Getters
    int get_port() const { return port_; }
    const std::map<std::string, HandlerConfig>& get_routes() const { return routes_; }
    
private:
    int port_ = 8080;
    std::map<std::string, HandlerConfig> routes_;  // path -> handler config
    
    // Helper to parse handler config from nginx block
    HandlerConfig parse_handler_config(const NginxConfig& block);
};

#endif