#pragma once
#include "socket_state.h"

class WebServer {
public:
	WebServer();
	~WebServer();
	void run();
private:
	static constexpr int LISTENER_PORT = 80 ;
	static constexpr int MAX_SOCKETS = 16;
	static constexpr int MAX_BACKLOG = 4;

	WSAData wsa_data;
	SOCKET listener_socket;
	sockaddr_in server_service;
	SocketState server_sockets[MAX_SOCKETS];
	size_t socket_count = 0;
	fd_set wait_recv, wait_send;
	int nfd;

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

	bool add_socket(SOCKET id, int what);
	void remove_socket(size_t index);

	void accept_connection(size_t index);
	void receive_message(size_t index);
	void send_message(size_t index);
};