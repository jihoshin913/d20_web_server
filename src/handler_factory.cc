#include "handler_factory.h"
#include "request_handler.h"
#include "echo_handler.h"
#include "file_handler.h"
#include "not_found_handler.h"
#include "server_config.h"
#include "crud_handler.h"
#include "health_handler.h"
#include "sleep_handler.h"
#include "mock_filesystem.h"
#include <sstream>

std::unique_ptr<RequestHandler> HandlerFactory::create_handler(const HandlerConfig& config, std::string path) const {
        if (config.type == "EchoHandler") {
            return std::make_unique<EchoHandler>();
        } 
        else if (config.type == "HealthHandler") {
            return std::make_unique<HealthHandler>();
        }
        else if (config.type == "StaticHandler") {
            auto it = config.settings.find("root");
            if (it == config.settings.end()) { // Check if root path exists
                return nullptr;
            }
            std::string root = it->second;

            auto ext_it = config.settings.find("supported_extensions");
            if (ext_it != config.settings.end()) {
                std::unordered_set<std::string> extensions = parse_extensions(ext_it->second);
                return std::make_unique<FileHandler>(root, path, extensions);
            } else {
                return std::make_unique<FileHandler>(root, path);
            }
        }
        else if (config.type == "CrudHandler") {
            static std::shared_ptr<MockFilesystem> crud_fs = std::make_shared<MockFilesystem>();
            return std::make_unique<CrudHandler>(path, crud_fs);
        }
        else if (config.type == "SleepHandler") {
            // Default to 5 seconds, but allow override from config
            int sleep_seconds = 5;
            auto it = config.settings.find("sleep_seconds");
            if (it != config.settings.end()) {
                sleep_seconds = std::stoi(it->second);
            }
            return std::make_unique<SleepHandler>(sleep_seconds);
        }
        else {
            return std::make_unique<NotFoundHandler>();
        }
}

// Helper function to parse comma-separated extensions
std::unordered_set<std::string> HandlerFactory::parse_extensions(const std::string& ext_string) const {
    std::unordered_set<std::string> extensions;
    std::istringstream stream(ext_string);
    std::string ext;
    
    while (std::getline(stream, ext, ',')) {
        // Trim whitespace
        ext.erase(0, ext.find_first_not_of(" \t"));
        ext.erase(ext.find_last_not_of(" \t") + 1);
        
        if (!ext.empty()) {
            if (ext[0] != '.') {
                ext = "." + ext; 
            }
            extensions.insert(ext); 
        }
    }
    
    return extensions;
}
