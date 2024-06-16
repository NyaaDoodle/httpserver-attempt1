#pragma once
#include <string>
#include <map>
#include <vector>

class HTTPRequest {
public:
	HTTPRequest(const char* parse_buffer);
	const std::string& get_method() const;
	const std::string& get_uri() const;
	const std::string& get_http_version() const;
	const std::map<std::string, std::string>& get_headers() const;
	const std::string& get_body() const;
private:
	std::string method;
	std::string uri;
	std::string http_version;
	std::map<std::string, std::string> headers;
	std::string body;
};