#pragma once
#include <WinSock2.h>

struct SocketState {
	enum { EMPTY, LISTEN, RECEIVE, IDLE, SEND };
	enum { SEND_TIME = 1, SEND_SECONDS = 2 };
	static constexpr size_t BUFFER_LENGTH = 256;
	
	SOCKET id = 0;						// Socket handle
	int	recv = EMPTY;					// Receiving?
	int	send = EMPTY;					// Sending?
	int send_subtype = EMPTY;			// Sending sub-type
	char buffer[BUFFER_LENGTH] = {0};
	int length = 0;
};