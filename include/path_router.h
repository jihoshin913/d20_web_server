#ifndef PATH_ROUTER_H
#define PATH_ROUTER_H

#include "request_handler.h"
#include "echo_handler.h"
#include "file_handler.h"
#include "server_config.h"
#include "handler_factory.h"
#include <memory>
#include <string>

// path_router.h
class PathRouter {
public:
    PathRouter(const ServerConfig& config);
    std::unique_ptr<RequestHandler> match_handler(const std::string& path) const;
    
private:
    std::map<std::string, HandlerConfig> routes_;

    // Handler Factory
    HandlerFactory handler_factory_;
    
};

#endif