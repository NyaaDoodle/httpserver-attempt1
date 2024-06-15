#pragma once
#include <string>
#include <map>
#include <vector>

class HTTPRequest {
public:
	HTTPRequest(char* parse_buffer);
	std::string get_method() const;
	std::string get_request_uri() const;
	std::string get_http_version() const;
	std::map<std::string, std::string>& get_headers();
	std::string& get_body();
private:
	std::string method;
	std::string request_uri;
	std::string http_version;
	std::map<std::string, std::string> headers;
	std::string body;
	bool valid;
};

HTTPRequest::HTTPRequest(char* parse_buffer) {
	const char* CRLF = "\n";
	char* request_line = strtok(parse_buffer, CRLF);
	std::vector<char*> header_lines;
	char* temp;
	while ((temp = strtok(NULL, CRLF)) != "" && temp != NULL) {
		header_lines.push_back(temp);
	}
	body = parse_buffer;
}