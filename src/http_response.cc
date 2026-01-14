#include "http_response.h"

// Default constructor
HttpResponse::HttpResponse() : 
  version("HTTP/1.1"), message_body(""), headers_map({{"Content-Length", "0"}}) {}

HttpResponse::HttpResponse(const std::string& v, int sc, const std::string& rp, 
                           const std::map<std::string, std::string>& hm, const std::string& mb) 
  : version(v), status_code(sc), reason_phrase(rp), headers_map(hm), message_body(mb) {
    std::string content_length_str = std::to_string(message_body.size());
    set_header("Content-Length", content_length_str);
}


// Setters
void HttpResponse::set_version(const std::string& v){
  version = v;
}

void HttpResponse::set_status_code(int sc){
  status_code = sc;
}

void HttpResponse::set_reason_phrase(const std::string& rp){
  reason_phrase = rp;
}

void HttpResponse::set_header(const std::string& k, const std::string& v){
  headers_map[k] = v;
}

void HttpResponse::set_message_body(const std::string& mb){
  message_body = mb;

  std::string content_length_str = std::to_string(message_body.size());
  set_header("Content-Length", content_length_str);
}

//Getters
std::string HttpResponse::get_version() const {
  return version;
}

int HttpResponse::get_status_code() const {
  return status_code;
}

std::string HttpResponse::get_reason_phrase() const {
  return reason_phrase;
}

std::string HttpResponse::get_header(const std::string& header_name) const {
  auto it = headers_map.find(header_name);
  if (it != headers_map.end()) {
      return it->second;
  }
  return "";
}

std::string HttpResponse::get_message_body() const {
  return message_body;
}

//Methods
std::string HttpResponse::convert_to_string() const{

  // Construct the Status line 
  std::string status_line = version + " " + std::to_string(status_code)
                                    + " " + reason_phrase + "\r\n";

  // Construct the Header lines
  std::string headers_str = "";
  for (auto const& [header_name, header_value] : headers_map) {
    headers_str += header_name + ": " + header_value + "\r\n";
  }

  // Put it all together
  std::string response_string = status_line +
                                headers_str + 
                                "\r\n" +
                                message_body; 

  return response_string;

}
