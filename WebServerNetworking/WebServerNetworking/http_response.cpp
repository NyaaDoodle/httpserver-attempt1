#define _CRT_SECURE_NO_WARNINGS
#include "http_response.h"

const std::map<int, std::string> HTTPResponse::reason_phrase = {
        {100, "Continue"},
        {101, "Switching Protocols"},
        {200, "OK"},
        {201, "Created"},
        {203, "Non-Authoritative Information"},
        {204, "No Content"},
        {205, "Reset Content"},
        {206, "Partial Content"},
        {300, "Multiple Choices"},
        {301, "Moved Permanently"},
        {302, "Found"},
        {303, "See Other"},
        {304, "Not Modified"},
        {305, "Use Proxy"},
        {307, "Temporary Redirect"},
        {400, "Bad Request"},
        {401, "Unauthorized"},
        {402, "Payment Required"},
        {403, "Forbidden"},
        {404, "Not Found"},
        {405, "Method Not Allowed"},
        {406, "Not Acceptable"},
        {407, "Proxy Authentication Required"},
        {408, "Request Time-out"},
        {409, "Conflict"},
        {410, "Gone"},
        {411, "Length Required"},
        {412, "Precondition Failed"},
        {413, "Request Entity Too Large"},
        {414, "Request-URI Too Large"},
        {415, "Unsupported Media Type"},
        {416, "Requested range not satisfiable"},
        {417, "Expectation Failed"},
        {500, "Internal Server Error"},
        {501, "Not Implemented"},
        {502, "Bad Gateway"},
        {503, "Service Unavailable"},
        {504, "Gateway Time-out"},
        {505, "HTTP Version not supported"}
};

HTTPResponse::HTTPResponse(const int status_code, const std::string& http_version, const std::string& body) {
    set_status_code(status_code);
    set_http_version(http_version);
    set_body(body);
    time(&timer);
    std::string dt = ctime(&timer);
    dt.pop_back();
    insert_header("Date", dt);
}

const std::string& HTTPResponse::get_http_version() const { return http_version; }
const int HTTPResponse::get_status_code() const { return status_code; }
const std::map<std::string, std::string>& HTTPResponse::get_headers() const { return headers; }
const std::string& HTTPResponse::get_body() const { return body; }
void HTTPResponse::set_http_version(const std::string& http_version) { this->http_version = http_version; }
void HTTPResponse::set_status_code(const int status_code) { this->status_code = status_code; }
void HTTPResponse::insert_header(const std::string& key, const std::string& value) { headers[key] = value; }
void HTTPResponse::set_body(const std::string& body) { 
    this->body = body;
    update_content_length();
}
void HTTPResponse::append_to_body(const std::string& append) { 
    body.append(append);
    update_content_length();
}
void HTTPResponse::pop_back_n_body(const size_t n) {
    for (size_t i = 0; i < n; i++) {
        body.pop_back();
    }
    update_content_length();
}
std::string HTTPResponse::write_response() const {
    const char* CRLF = "\r\n";
    const char* SP = " ";
    const char* COLON_SP = ": ";
    std::string response;
    response.append(http_version);
    response.append(SP);
    response.append(std::to_string(status_code));
    response.append(SP);
    response.append(reason_phrase.at(status_code));
    response.append(CRLF);
    for (auto it = headers.begin(); it != headers.end(); it++) {
        response.append(it->first);
        response.append(COLON_SP);
        response.append(it->second);
        response.append(CRLF);
    }
    response.append(CRLF);
    response.append(body);
    return response;
}
void HTTPResponse::update_content_length() {
    insert_header("Content-Length", std::to_string(body.size()));
}