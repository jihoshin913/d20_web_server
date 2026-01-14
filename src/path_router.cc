#include "path_router.h"
#include "request_handler.h"
#include "echo_handler.h"
#include "file_handler.h"
#include "server_config.h"
#include <sstream>

PathRouter::PathRouter(const ServerConfig& config) 
    : routes_(config.get_routes()) {}

std::unique_ptr<RequestHandler> PathRouter::match_handler(const std::string& path) const {
    std::string prefix_match = "";
    int max_len = 0;
    
    for (const auto& [route_path, handler_config] : routes_) {
        if (path.rfind(route_path, 0) == 0) {
            if (route_path.length() > max_len) {
                max_len = route_path.length();
                prefix_match = route_path;
            }
        }
    }
    if (!prefix_match.empty()){
        const auto& handler = routes_.at(prefix_match);
        return handler_factory_.create_handler(handler, prefix_match);
    }

    // Default fallback
    HandlerConfig not_found_config;
    not_found_config.type = "NotFoundHandler";
    return handler_factory_.create_handler(not_found_config, path);
}
