#ifndef HTTP_HELPER_H
#define HTTP_HELPER_H

#include <string>
#include <boost/asio.hpp>
#include "http_response.h"

enum class MalformedType {
    None,
    Empty,
    NoHeaderTerminator
};

bool detect_http_request(std::string& buffer);

MalformedType check_malformed_request(const std::string& buffer);

#endif
