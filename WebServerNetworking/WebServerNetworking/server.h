#pragma once
#include "socket_state.h"
#include "http_request.h"
#include "http_response.h"
#include <string>
#include <map>

class WebServer {
public:
	WebServer();
	~WebServer();
	void run();
private:
	enum { ROOT, ASTERISK, INDEX, LIST, DB, UNKNOWN };
	static constexpr int LISTENER_PORT = 80;
	static constexpr int MAX_SOCKETS = 16;
	static constexpr int MAX_BACKLOG = 4;
	static constexpr int SEND_BUFFER_SIZE = 2048;
	static const std::map<std::string, int> method_to_action;
	static const std::map<std::string, int> uri_to_resource;
	static const std::string RESOURCES_DIR;

	WSAData wsa_data;
	SOCKET listener_socket;
	sockaddr_in server_service;
	SocketState server_sockets[MAX_SOCKETS];
	size_t socket_count = 0;
	fd_set wait_recv, wait_send;
	int nfd;
	char send_buffer[SEND_BUFFER_SIZE];
	time_t server_timer;

	std::vector<std::string> list;
	std::map<std::string, std::string> db;

	void init_winsock();
	void create_listener_socket();
	void bind_listener_socket();
	void start_listening();
	void server_loop();
	void close_server();

	void setup_fd_sets();
	void select_fd_sets();
	void recv_act_on_select();
	void send_act_on_select();
	void timeout_sockets();

	bool add_socket(SOCKET id, int what);
	void remove_socket(const size_t index);

	void accept_connection(const size_t index);
	void receive_message(const size_t index);
	void send_message(const size_t index);
	void do_action(const size_t index);

	void do_options(const HTTPRequest& request);
	void do_get(const HTTPRequest& request, const bool head_req = false);
	void do_head(const HTTPRequest& request);
	void do_post(const HTTPRequest& request);
	void do_put(const HTTPRequest& request);
	void do_delete(const HTTPRequest& request);
	void do_trace(const HTTPRequest& request);
	void unsupported_request(const HTTPRequest& request);
	void resource_not_found(const HTTPRequest& request);

	void write_to_send_buffer(const HTTPResponse& response);
	void file_to_body(HTTPResponse& respond, std::string file_name);
	void list_to_body(HTTPResponse& response);
	void db_to_body(HTTPResponse& response);
};