// Disclaimer: Most of this class is from Claude Sonnet 4.5, with some of my modifications (Tony)
#include "http_request.h"
#include <sstream>
#include <algorithm>
#include <cctype>

HttpRequest HttpRequest::parse(const std::string& raw_request) {
    HttpRequest request;
    request.raw_request_ = raw_request;
    std::istringstream stream(raw_request);
    
    // Parse request line: "GET /path HTTP/1.1"
    if (!(stream >> request.method_ >> request.path_ >> request.version_)) {
        // Return invalid request
        request.method_ = "";
        request.path_ = "";
        request.version_ = "";
        return request;
    }

    request.parse_path_and_query();
    
    // Consume the rest of the request line
    std::string line;
    std::getline(stream, line);
    
    // Parse headers
    while (std::getline(stream, line)) {
        // Remove \r if present
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        
        // Empty line marks end of headers
        if (line.empty()) {
            break;
        }
        
        // Parse "Header-Name: value"
        size_t colon_pos = line.find(':');
        if (colon_pos != std::string::npos) {
            std::string name = line.substr(0, colon_pos);
            std::string value = line.substr(colon_pos + 1);
            
            // Trim whitespace from value
            value = trim(value);
            
            request.headers_[name] = value;
        }
    }
    
    // Parse body (everything after headers)
    std::ostringstream body_stream;
    while (std::getline(stream, line)) {
        body_stream << line << "\n";
    }
    std::string body_content = body_stream.str();
    
    // Remove trailing newline if present
    if (!body_content.empty() && body_content.back() == '\n') {
        body_content.pop_back();
    }
    
    request.body_ = body_content;
    
    return request;
}

std::optional<std::string> HttpRequest::get_header(const std::string& name) const {
    // Case-insensitive header lookup
    for (const auto& [key, value] : headers_) {
        if (to_lower(key) == to_lower(name)) {
            return value;
        }
    }
    return std::nullopt;
}

bool HttpRequest::has_header(const std::string& name) const {
    return get_header(name).has_value();
}

void HttpRequest::add_header(const std::string& name, const std::string& value) {
    headers_[name] = value;
}


bool HttpRequest::is_valid() const { 
    // Basic field presence check
    if (method_.empty() || path_.empty() || version_.empty()) {
        return false;
    }

    // Validate HTTP version
    if (version_ != "HTTP/1.1") {
        return false;
    }

    // Validate HTTP method
    static const std::vector<std::string> valid_methods = {
        "GET", "POST", "PUT", "DELETE"
    };
    if (std::find(valid_methods.begin(), valid_methods.end(), method_) == valid_methods.end()) {
        return false;
    }

    // Validate path starts with /
    if (path_[0] != '/') {
        return false;
    }

    // Validate path doesn't contain any spaces
    if (path_.find(' ') != std::string::npos) {
        return false;
    }

    //  Check for single blank line after headers
    size_t headers_end = raw_request_.find("\r\n\r\n");
    if (headers_end == std::string::npos) {
        return false;
    }

    return true;

}

std::string HttpRequest::to_string() const {
    std::ostringstream oss;
    oss << "Method: " << method_ << "\n";
    oss << "Path: " << path_ << "\n";
    oss << "Version: " << version_ << "\n";
    
    if (!headers_.empty()) {
        oss << "Headers:\n";
        for (const auto& [name, value] : headers_) {
            oss << "  " << name << ": " << value << "\n";
        }
    }
    
    if (!body_.empty()) {
        oss << "Body:\n" << body_ << "\n";
    }
    
    return oss.str();
}

std::string HttpRequest::trim(const std::string& str) {
    size_t start = str.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) {
        return "";
    }
    
    size_t end = str.find_last_not_of(" \t\r\n");
    return str.substr(start, end - start + 1);
}

std::string HttpRequest::to_lower(const std::string& str) {
    std::string result = str;
    std::transform(result.begin(), result.end(), result.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    return result;
}

void HttpRequest::parse_path_and_query() {
    size_t query_pos = path_.find('?');
    
    if (query_pos == std::string::npos) {
        base_path_ = path_;
        return;
    }
    
    base_path_ = path_.substr(0, query_pos);
    std::string query_string = path_.substr(query_pos + 1);
    
    std::istringstream query_stream(query_string);
    std::string pair;
    
    while (std::getline(query_stream, pair, '&')) {
        size_t equals_pos = pair.find('=');
        if (equals_pos != std::string::npos) {
            std::string key = pair.substr(0, equals_pos);
            std::string value = pair.substr(equals_pos + 1);
            query_params_[key] = value;
        }
    }
}

std::map<std::string, std::string> HttpRequest::get_query_params() const {
    return query_params_;
}

std::optional<std::string> HttpRequest::get_query_param(const std::string& key) const {
    auto it = query_params_.find(key);
    if (it != query_params_.end()) {
        return it->second;
    }
    return std::nullopt;
}
