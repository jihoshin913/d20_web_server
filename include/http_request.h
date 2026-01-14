#ifndef HTTP_REQUEST_H
#define HTTP_REQUEST_H

#include <string>
#include <map>
#include <vector>
#include <optional>
#include <algorithm>

class HttpRequest {
public:
    // Constructors
    HttpRequest() = default;
    
    // Parse from raw HTTP request string
    static HttpRequest parse(const std::string& raw_request);
    
    // Getters
    const std::string& method() const { return method_; }
    const std::string& path() const { return path_; }
    const std::string base_path() const { return base_path_; }
    const std::string& version() const { return version_; }
    const std::string& body() const { return body_; }
    const std::string& raw_request() const { return raw_request_; }
    
    // Header access
    std::optional<std::string> get_header(const std::string& name) const;
    bool has_header(const std::string& name) const;
    const std::map<std::string, std::string>& headers() const { return headers_; }
    
    // Utility methods
    bool is_valid() const;
    std::string full_path() const; // path + query string if present
    
    // Setters (for manual construction or testing)
    void set_method(const std::string& method) { method_ = method; }
    void set_path(const std::string& path) { 
        path_ = path; 
        parse_path_and_query();
    }
    void set_version(const std::string& version) { version_ = version; }
    void set_body(const std::string& body) { body_ = body; }
    void add_header(const std::string& name, const std::string& value);

    // Param methods
    std::map<std::string, std::string> get_query_params() const;
    std::optional<std::string> get_query_param(const std::string& key) const;
    
    // Debug
    std::string to_string() const;

private:
    std::string method_;
    std::string path_;
    std::string base_path_;
    std::string version_;
    std::string body_;
    std::map<std::string, std::string> headers_;
    std::string raw_request_;

    std::map<std::string, std::string> query_params_;

    // Helper methods
    static std::string trim(const std::string& str);
    static std::string to_lower(const std::string& str);

    void parse_path_and_query();
};

#endif // HTTP_REQUEST_H
