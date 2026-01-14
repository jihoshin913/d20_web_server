#ifndef HTTP_RESPONSE_H
#define HTTP_RESPONSE_H

#include <string>
#include <map>

class HttpResponse
{

public:
  
  // Default constructor
  HttpResponse();

  // Non-default constructor
  HttpResponse(const std::string& v, int sc, const std::string& rp, 
                const std::map<std::string, std::string>& hm, const std::string& mb);

  // Setters
  void set_version(const std::string& v);
  void set_status_code(int sc);
  void set_reason_phrase(const std::string& rp);

  void set_header(const std::string& k, const std::string& v);

  void set_message_body(const std::string& mb);


  // Getters
  std::string get_version() const;
  int get_status_code() const;
  std::string get_reason_phrase() const;

  std::string get_header(const std::string& header_name) const;

  std::string get_message_body() const;

  // Methods
  std::string convert_to_string() const;

private:

  // Status line components

  std::string version;
  int status_code;
  std::string reason_phrase;

  // Header line components
  std::map<std::string, std::string> headers_map;
  
  // Message body components
  std::string message_body;

};

#endif
