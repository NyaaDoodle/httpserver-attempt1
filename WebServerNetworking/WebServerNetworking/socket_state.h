#pragma once
#include <WinSock2.h>
#include "http_request.h"
#include <time.h>

struct SocketState {
	enum { EMPTY, LISTEN, RECEIVE, IDLE, SEND };
	enum { NONE, OPTIONS, GET, HEAD, POST, PUT, DEL, TRACE, UNSUPPORTED };
	static constexpr size_t BUFFER_LENGTH = 2048;
	
	SOCKET id = 0;						// Socket handle
	int	recv = EMPTY;					// Receiving?
	int	send = EMPTY;					// Sending?
	int send_action = NONE;				// Sending sub-type
	HTTPRequest request;
	char buffer[BUFFER_LENGTH] = {0};
	int length = 0;
	time_t recv_timer;
};