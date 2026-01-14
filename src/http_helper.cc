#include "http_helper.h"
#include <sstream>

bool detect_http_request(std::string& buffer) {
  return buffer.find("\r\n\r\n") != std::string::npos || buffer.find("\n\n") != std::string::npos;
}

MalformedType check_malformed_request(const std::string& buffer) {
    if (buffer.empty()) {
        return MalformedType::Empty;
    }
    if (buffer.find("\r\n\r\n") == std::string::npos) {
        return MalformedType::NoHeaderTerminator;
    }
    return MalformedType::None;
}
