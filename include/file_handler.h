#ifndef FILE_HANDLER_H
#define FILE_HANDLER_H

#include "http_response.h"
#include "request_handler.h"
#include <string>
#include <unordered_set>

// This class handles any requests by redirecting the request and respond with file requested
// This implements the functionality for the webserver from 
// Assignment 4. 
// Inherits from the abstract RequestHandler class.
class FileHandler : public RequestHandler {
  public:
    FileHandler(const std::string& root, const std::string& route_prefix);
    
    // Alternative constructor with custom supported file extensions
    FileHandler(const std::string& root, 
                const std::string& route_prefix,
                const std::unordered_set<std::string>& supported_extensions);

    HttpResponse handle_request(const HttpRequest& request) override;

    std::string get_handler_name() const override;

  private:
    std::string root_;  
    std::string route_prefix_;
    std::unordered_set<std::string> supported_extensions_; 
    
    std::string get_mime_type(const std::string& file_path) const;
    
    bool is_supported_file_type(const std::string& file_path) const;
    
    std::string sanitize_path(const std::string& path) const;
};

#endif
