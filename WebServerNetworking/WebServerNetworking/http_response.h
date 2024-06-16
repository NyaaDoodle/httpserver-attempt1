#pragma once
#include <string>
#include <map>

class HTTPResponse {
public:
	HTTPResponse(const int status_code, const std::string& http_version = "HTTP/1.1", const std::string& body = "")
		: status_code(status_code), http_version(http_version), body(body) {}
	const std::string& get_http_version() const;
	const int get_status_code() const;
	const std::map<std::string, std::string>& get_headers() const;
	const std::string& get_body() const;
	void set_http_version(const std::string& http_version);
	void set_status_code(const int status_code);
	void insert_header(const std::string& key, const std::string& value);
	void set_body(const std::string& body);
	std::string write_response();
private:
	static const std::map<int, std::string> reason_phrase;
	std::string http_version;
	int status_code;
	std::map<std::string, std::string> headers;
	std::string body = "";
};