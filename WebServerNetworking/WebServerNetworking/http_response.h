#pragma once
#include <string>
#include <map>
#include <time.h>

class HTTPResponse {
public:
	HTTPResponse(const int status_code = 0, const std::string& http_version = "HTTP/1.1", const std::string& body = "");
	HTTPResponse(const HTTPResponse& response) = default;
	HTTPResponse& operator=(const HTTPResponse& response) = default;
	const std::string& get_http_version() const;
	const int get_status_code() const;
	const std::map<std::string, std::string>& get_headers() const;
	const std::string& get_body() const;
	void set_http_version(const std::string& http_version);
	void set_status_code(const int status_code);
	void insert_header(const std::string& key, const std::string& value);
	void set_body(const std::string& body = "");
	void append_to_body(const std::string& append);
	void pop_back_n_body(const size_t n);
	std::string write_response() const;
private:
	static const std::string HOSTNAME;
	static constexpr int LISTENER_PORT = 80;
	static const std::map<int, std::string> reason_phrase;
	std::string http_version;
	int status_code;
	std::map<std::string, std::string> headers;
	std::string body = "";
	time_t timer;
	void update_content_length();
};