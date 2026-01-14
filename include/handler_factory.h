#ifndef HANDLER_FACTORY_H
#define HANDLER_FACTORY_H

#include "request_handler.h"
#include "server_config.h"
#include <memory>
#include <string>

class HandlerFactory {
public:
    std::unique_ptr<RequestHandler> create_handler(const HandlerConfig& config, std::string path) const;

    // Helper to parse comma-separated extensions
    std::unordered_set<std::string> parse_extensions(const std::string& ext_string) const;
};

#endif