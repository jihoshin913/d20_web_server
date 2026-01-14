#include "http_response.h"
#include "health_handler.h"

HttpResponse HealthHandler::handle_request(const HttpRequest& request){

  HttpResponse response = HttpResponse("HTTP/1.1", 200, "OK", 
                                        {{"Content-Type", "text/plain"}},
                                        "OK");

  return response;

}

std::string HealthHandler::get_handler_name() const {
  return "HealthHandler";
}
