#define _CRT_SECURE_NO_WARNINGS
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include "server.h"
#include <iostream>
#include <winsock2.h>
#include <string.h>
#include <time.h>
#include "server_exception.h"
#pragma comment(lib, "Ws2_32.lib")

WebServer::WebServer() {
	init_winsock();
	create_listener_socket();
	bind_listener_socket();
}

WebServer::~WebServer() {
	close_server();
}

void WebServer::init_winsock() {
	if (NO_ERROR != WSAStartup(MAKEWORD(2, 2), &wsa_data)) {
		std::cout << "Web Server: Error at WSAStartup()\n";
		throw InitWinSockException();
	}
}

void WebServer::create_listener_socket() {
	listener_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (INVALID_SOCKET == listener_socket) {
		std::cout << "Web Server: Error at socket(): " << WSAGetLastError() << std::endl;
		WSACleanup();
		throw CreateListenerSocketException();
	}
}

void WebServer::bind_listener_socket() {
	server_service.sin_family = AF_INET;
	server_service.sin_addr.s_addr = INADDR_ANY;
	server_service.sin_port = htons(LISTENER_PORT);
	if (SOCKET_ERROR == bind(listener_socket, (SOCKADDR*)&server_service, sizeof(server_service))) {
		std::cout << "Web Server: Error at bind(): " << WSAGetLastError() << std::endl;
		closesocket(listener_socket);
		WSACleanup();
		throw BindListenerSocketException();
	}
}

void WebServer::start_listening() {
	if (SOCKET_ERROR == listen(listener_socket, MAX_BACKLOG)) {
		std::cout << "Web Server: Error at listen(): " << WSAGetLastError() << std::endl;
		closesocket(listener_socket);
		WSACleanup();
		throw StartListenerException();
	}
	add_socket(listener_socket, SocketState::LISTEN);
}

void WebServer::run() {
	start_listening();
	server_loop();
}

void WebServer::server_loop() {
	while (true) {
		setup_fd_sets();
		select_fd_sets();
		recv_act_on_select();
		send_act_on_select();
	}
}

void WebServer::close_server() {
	std::cout << "Web Server: Closing Connection.\n";
	closesocket(listener_socket);
	WSACleanup();
}

void WebServer::setup_fd_sets() {
	FD_ZERO(&wait_recv);
	for (int i = 0; i < MAX_SOCKETS; i++) {
		if ((server_sockets[i].recv == SocketState::LISTEN) || (server_sockets[i].recv == SocketState::RECEIVE)) {
			FD_SET(server_sockets[i].id, &wait_recv);
		}
	}
	FD_ZERO(&wait_send);
	for (int i = 0; i < MAX_SOCKETS; i++) {
		if (server_sockets[i].send == SocketState::SEND) {
			FD_SET(server_sockets[i].id, &wait_send);
		}
	}
}

void WebServer::select_fd_sets() {
	nfd = select(0, &wait_recv, &wait_send, NULL, NULL);
	if (nfd == SOCKET_ERROR) {
		std::cout << "Web Server: Error at select(): " << WSAGetLastError() << std::endl;
		WSACleanup();
		throw SelectException();
	}
}

void WebServer::recv_act_on_select() {
	for (size_t i = 0; i < MAX_SOCKETS && nfd > 0; i++) {
		if (FD_ISSET(server_sockets[i].id, &wait_recv)) {
			nfd--;
			switch (server_sockets[i].recv) {
			case SocketState::LISTEN:
				accept_connection(i);
				break;
			case SocketState::RECEIVE:
				receive_message(i);
				break;
			}
		}
	}
}

void WebServer::send_act_on_select() {
	for (size_t i = 0; i < MAX_SOCKETS && nfd > 0; i++) {
		if (FD_ISSET(server_sockets[i].id, &wait_send)) {
			nfd--;
			switch (server_sockets[i].send) {
			case SocketState::SEND:
				send_message(i);
				break;
			}
		}
	}
}

bool WebServer::add_socket(SOCKET id, int what) {
	for (int i = 0; i < MAX_SOCKETS; i++) {
		if (server_sockets[i].recv == SocketState::EMPTY) {
			server_sockets[i].id = id;
			server_sockets[i].recv = what;
			server_sockets[i].send = SocketState::IDLE;
			server_sockets[i].length = 0;
			socket_count++;
			return true;
		}
	}
	return false;
}

void WebServer::remove_socket(size_t index) {
	server_sockets[index].recv = SocketState::EMPTY;
	server_sockets[index].send = SocketState::EMPTY;
	socket_count--;
}

void WebServer::accept_connection(size_t index) {
	SOCKET id = server_sockets[index].id;
	struct sockaddr_in from;
	int fromLen = sizeof(from);
	unsigned long flag = 1; // Set the socket to be in non-blocking mode.

	SOCKET msgSocket = accept(id, (struct sockaddr *)&from, &fromLen);
	if (INVALID_SOCKET == msgSocket) { 
		 std::cout << "Web Server: Error at accept(): " << WSAGetLastError() << std::endl; 		 
		 return;
	}
	std::cout << "Web Server: Client "<<inet_ntoa(from.sin_addr)<<":"<<ntohs(from.sin_port)<<" is connected." << std::endl;
	if (ioctlsocket(msgSocket, FIONBIO, &flag) != 0) {
		std::cout << "Web Server: Error at ioctlsocket(): "<<WSAGetLastError() << std::endl;
	}

	if (add_socket(msgSocket, SocketState::RECEIVE) == false) {
		std::cout << "\t\tToo many connections, dropped!\n";
		closesocket(id);
	}
}

void WebServer::receive_message(size_t index) {
	SOCKET msgSocket = server_sockets[index].id;

	int len = server_sockets[index].length;
	int bytesRecv = recv(msgSocket, &server_sockets[index].buffer[len], sizeof(server_sockets[index].buffer) - len, 0);
	
	if (SOCKET_ERROR == bytesRecv) {
		std::cout << "Web Server: Error at recv(): " << WSAGetLastError() << std::endl;
		closesocket(msgSocket);			
		remove_socket(index);
		return;
	}
	if (bytesRecv == 0) {
		closesocket(msgSocket);			
		remove_socket(index);
		return;
	}
	else {
		server_sockets[index].buffer[len + bytesRecv] = '\0'; //add the null-terminating to make it a string
		std::cout << "Web Server: Recieved: "<< bytesRecv <<" bytes of \"" << (&server_sockets[index].buffer[len]) << "\" message.\n";
		
		server_sockets[index].length += bytesRecv;

		if (server_sockets[index].length > 0) {
			if (strncmp(server_sockets[index].buffer, "TimeString", 10) == 0)
			{
				server_sockets[index].send  = SocketState::SEND;
				server_sockets[index].send_subtype = SocketState::SEND_TIME;
				memcpy(server_sockets[index].buffer, &server_sockets[index].buffer[10], server_sockets[index].length - 10);
				server_sockets[index].length -= 10;
				return;
			}
			else if (strncmp(server_sockets[index].buffer, "SecondsSince1970", 16) == 0)
			{
				server_sockets[index].send  = SocketState::SEND;
				server_sockets[index].send_subtype = SocketState::SEND_SECONDS;
				memcpy(server_sockets[index].buffer, &server_sockets[index].buffer[16], server_sockets[index].length - 16);
				server_sockets[index].length -= 16;
				return;
			}
			else if (strncmp(server_sockets[index].buffer, "Exit", 4) == 0)
			{
				closesocket(msgSocket);
				remove_socket(index);
				return;
			}
		}
	}

}

void WebServer::send_message(size_t index) {
	int bytesSent = 0;
	char sendBuff[255];

	SOCKET msgSocket = server_sockets[index].id;
	if (server_sockets[index].send_subtype == SocketState::SEND_TIME) {
		time_t timer;
		time(&timer);
		strcpy(sendBuff, ctime(&timer));
		sendBuff[strlen(sendBuff)-1] = 0;
	}
	else if(server_sockets[index].send_subtype == SocketState::SEND_SECONDS) {
		time_t timer;
		time(&timer);
		_itoa((int)timer, sendBuff, 10);		
	}

	bytesSent = send(msgSocket, sendBuff, (int)strlen(sendBuff), 0);
	if (SOCKET_ERROR == bytesSent) {
		std::cout << "Web Server: Error at send(): " << WSAGetLastError() << std::endl;	
		return;
	}

	std::cout<<"Web Server: Sent: "<<bytesSent<<"\\"<<strlen(sendBuff)<<" bytes of \""<<sendBuff<<"\" message.\n";	
	server_sockets[index].send = SocketState::IDLE;
}