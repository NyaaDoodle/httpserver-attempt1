#include "http_request.h"

HTTPRequest::HTTPRequest(const char* parse_buffer) {
	enum State { METHOD, URI, HTTPVERSION, HEADER_KEY, HEADER_VALUE, BODY };
	State state = METHOD;
	std::string header_key, header_value;
	for (size_t i = 0; i < strlen(parse_buffer); i++) {
		switch (state) {
		case METHOD:
			if (parse_buffer[i] != ' ') {
				method.push_back(parse_buffer[i]);
			}
			else {
				state = URI;
			}
			break;
		case URI:
			if (parse_buffer[i] != ' ') {
				uri.push_back(parse_buffer[i]);
			}
			else {
				state = HTTPVERSION;
			}
			break;
		case HTTPVERSION:
			if (!(parse_buffer[i] == '\r' && parse_buffer[i + 1] == '\n')) {
				http_version.push_back(parse_buffer[i]);
			}
			else {
				i += 2;
				if (parse_buffer[i] == '\r' && parse_buffer[i + 1] == '\n') {
					state = BODY;
				}
				else {
					state = HEADER_KEY;
					i--;
				}
			}
			break;
		case HEADER_KEY:
			if (!(parse_buffer[i] == ':' && parse_buffer[i + 1] == ' ')) {
				header_key.push_back(parse_buffer[i]);
			}
			else {
				state = HEADER_VALUE;
				i++;
			}
			break;
		case HEADER_VALUE:
			if (!(parse_buffer[i] == '\r' && parse_buffer[i + 1] == '\n')) {
				header_value.push_back(parse_buffer[i]);
			}
			else {
				headers[header_key] = header_value;
				header_key.clear();
				header_value.clear();
				i += 2;
				if (parse_buffer[i] == '\r' && parse_buffer[i + 1] == '\n') {
					state = BODY;
					i++;
				}
				else {
					state = HEADER_KEY;
					i--;
				}
			}
			break;
		case BODY:
			body.push_back(parse_buffer[i]);
			break;
		}
	}
}

const std::string& HTTPRequest::get_method() const { return method; }
const std::string& HTTPRequest::get_uri() const { return uri; }
const std::string& HTTPRequest::get_http_version() const { return http_version; }
const std::map<std::string, std::string>& HTTPRequest::get_headers() const { return headers; }
const std::string& HTTPRequest::get_body() const { return body; }