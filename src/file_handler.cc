// Disclaimer: The main backbone of this class was written by Claude Sonnet 4.5, and the functionalities and detailed interface design 
// are checked and adjusted, which I also added and reduced functionality to conform with the asignment requirements. (Tony)
#include "file_handler.h"
#include <fstream>
#include <sstream>
#include <filesystem>
#include <algorithm>
#include <unordered_map>

// Default supported file extensions with their MIME types
static const std::unordered_set<std::string> DEFAULT_EXTENSIONS = {
    ".html", ".htm", ".css", ".js", ".json", 
    ".jpg", ".jpeg", ".png", ".gif", ".svg",
    ".txt", ".xml", ".pdf", ".ico", ".zip"
};

// Constructor with default supported extensions
FileHandler::FileHandler(const std::string& root, const std::string& route_prefix) 
    : root_(root), route_prefix_(route_prefix), supported_extensions_(DEFAULT_EXTENSIONS) {
    // Ensure root ends with '/' for consistent path joining
    if (!root_.empty() && root_.back() != '/') {
        root_ += '/';
    }
    
    if (!std::filesystem::exists(root_)) {
        throw std::invalid_argument("Root path does not exist: " + root_);
    }
    
    if (!std::filesystem::is_directory(root_)) {
        throw std::invalid_argument("Root path is not a directory: " + root_);
    }
}

// Constructor with custom supported extensions
FileHandler::FileHandler(const std::string& root, 
                         const std::string& route_prefix,
                         const std::unordered_set<std::string>& supported_extensions)
    : root_(root), route_prefix_(route_prefix), supported_extensions_(supported_extensions) {
    // Ensure root ends with '/' for consistent path joining
    if (!root_.empty() && root_.back() != '/') {
        root_ += '/';
    }

    if (!std::filesystem::exists(root_)) {
        throw std::invalid_argument("Root path does not exist: " + root_);
    }
    
    if (!std::filesystem::is_directory(root_)) {
        throw std::invalid_argument("Root path is not a directory: " + root_);
    }
}

std::string FileHandler::get_mime_type(const std::string& file_path) const {
    std::string extension = std::filesystem::path(file_path).extension().string();
    std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);
    
    // Common MIME type mappings
    static const std::unordered_map<std::string, std::string> mime_types = {
        {".html", "text/html"},
        {".htm", "text/html"},
        {".css", "text/css"},
        {".js", "application/javascript"},
        {".json", "application/json"},
        {".jpg", "image/jpeg"},
        {".jpeg", "image/jpeg"},
        {".png", "image/png"},
        {".gif", "image/gif"},
        {".svg", "image/svg+xml"},
        {".txt", "text/plain"},
        {".xml", "application/xml"},
        {".pdf", "application/pdf"},
        {".ico", "image/x-icon"},
        {".zip", "application/zip"}
        // Add more MIME type mappings as needed
    };
    
    auto it = mime_types.find(extension);
    if (it != mime_types.end()) {
        return it->second;
    }
    
    return "application/octet-stream";  // Default binary type
}

bool FileHandler::is_supported_file_type(const std::string& file_path) const {
    std::string extension = std::filesystem::path(file_path).extension().string();
    std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);
    
    return supported_extensions_.find(extension) != supported_extensions_.end();
}

// Sanitize file path to prevent directory traversal attacks
std::string FileHandler::sanitize_path(const std::string& path) const {
    std::string sanitized = path;
    
    // Remove any leading slashes
    while (!sanitized.empty() && sanitized[0] == '/') {
        sanitized = sanitized.substr(1);
    }
    
    // Remove any ".." or "." components
    std::vector<std::string> parts;
    std::stringstream ss(sanitized);
    std::string part;
    
    while (std::getline(ss, part, '/')) {
        if (part != ".." && part != "." && !part.empty()) {
            parts.push_back(part);
        }
    }
    
    // Reconstruct the path
    sanitized.clear();
    for (size_t i = 0; i < parts.size(); ++i) {
        sanitized += parts[i];
        if (i < parts.size() - 1) {
            sanitized += '/';
        }
    }
    
    return sanitized;
}

HttpResponse FileHandler::handle_request(const HttpRequest& request) {
    HttpResponse response;

    std::string path = request.path();
    std::string relative_path;
    size_t query = path.find('?');

    if (query != std::string::npos) {
        path.erase(query);
    }
    
    if (!route_prefix_.empty() && path.rfind(route_prefix_, 0) == 0) {
        relative_path = path.substr(route_prefix_.length());
        
        // If there's a leading slash after removal, keep it
        // If the relative path is empty, default to "/"
        if (relative_path.empty()) {
            relative_path = "/";
        }
    }
    
    relative_path = sanitize_path(relative_path);
    
    std::string full_path = root_ + relative_path;

    if (!std::filesystem::exists(full_path) || !std::filesystem::is_regular_file(full_path)) {
        response = HttpResponse("HTTP/1.1", 404, "Not Found", 
            {{"Content-Type", "text/html"}}, "<h1>404 Not Found</h1>");
        return response;
    }

    if (!is_supported_file_type(full_path)) {
        response = HttpResponse("HTTP/1.1", 415, "Unsupported Media Type", 
            {{"Content-Type", "text/plain"}}, "415 - Unsupported file type");
        return response;
    }

    std::ifstream file(full_path, std::ios::binary);
    if (!file.is_open()) {
        response = HttpResponse("HTTP/1.1", 500, "Internal Server Error", 
            {{"Content-Type", "text/plain"}}, "500 - Could not read file");
        return response;
    }

    std::stringstream buffer;
    buffer << file.rdbuf();

    response = HttpResponse("HTTP/1.1", 200, "OK", 
        {{"Content-Type", get_mime_type(full_path)}}, buffer.str());

    file.close();
    
    return response;
}

std::string FileHandler::get_handler_name() const {
    return "FileHandler";
}
