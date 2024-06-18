#pragma once
#include <string>
#include <map>
#include <vector>

class HTTPRequest {
public:
	HTTPRequest() = default;
	HTTPRequest(const char* parse_buffer);
	HTTPRequest(const HTTPRequest& request) = default;
	HTTPRequest& operator=(const HTTPRequest& request) = default;
	const std::string& get_method() const;
	const std::string& get_uri() const;
	const std::string& get_http_version() const;
	const std::map<std::string, std::string>& get_headers() const;
	const std::map<std::string, std::string>& get_queries() const;
	const std::string& get_body() const;
	const std::string& get_message_copy() const;
private:
	std::string method;
	std::string uri;
	std::string http_version;
	std::map<std::string, std::string> headers;
	std::map <std::string, std::string> queries;
	std::string body;
	std::string message_copy;
};