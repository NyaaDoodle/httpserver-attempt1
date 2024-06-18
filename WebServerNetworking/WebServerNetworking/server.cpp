#define _CRT_SECURE_NO_WARNINGS
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include "server.h"
#include <iostream>
#include <winsock2.h>
#include <string.h>
#include <time.h>
#include "server_exception.h"
#include "http_request.h"
#include <fstream>
#pragma comment(lib, "Ws2_32.lib")

const std::map<std::string, int> WebServer::method_to_action = {
	{"OPTIONS", SocketState::OPTIONS},
	{"GET", SocketState::GET},
	{"HEAD", SocketState::HEAD},
	{"POST", SocketState::POST},
	{"PUT", SocketState::PUT},
	{"DELETE", SocketState::DEL},
	{"TRACE", SocketState::TRACE}
};

const std::map<std::string, int> WebServer::uri_to_resource = {
	{"/", WebServer::ROOT},
	{"*", WebServer::ASTERISK},
	{"/index.html", WebServer::INDEX},
	{"/list", WebServer::LIST},
	{"/db", WebServer::DB},
};

const std::string WebServer::RESOURCES_DIR = "c:\\temp\\";

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
		timeout_sockets();
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
	time_t timer;
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
			time(&timer);
			server_sockets[i].recv_timer = timer;
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

void WebServer::timeout_sockets() {
	constexpr size_t SECONDS_IN_TWO_MINUTES = 120;
	time(&server_timer);
	for (size_t i = 0; i < MAX_SOCKETS; i++) {
		if (server_sockets[i].recv == SocketState::RECEIVE) {
			if ((server_timer - server_sockets[i].recv_timer) >= SECONDS_IN_TWO_MINUTES) {
				std::cout << "Socket " << i << " timed out, closing connection." << std::endl;
				closesocket(server_sockets[i].id);
				remove_socket(i);
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
			time(&server_sockets[i].recv_timer);
			socket_count++;
			return true;
		}
	}
	return false;
}

void WebServer::remove_socket(const size_t index) {
	server_sockets[index].recv = SocketState::EMPTY;
	server_sockets[index].send = SocketState::EMPTY;
	socket_count--;
}

void WebServer::accept_connection(const size_t index) {
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

void WebServer::receive_message(const size_t index) {
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
		server_sockets[index].request = HTTPRequest(&server_sockets[index].buffer[len]);
		if (server_sockets[index].length > 0) {
			server_sockets[index].send = SocketState::SEND;
			const auto it = method_to_action.find(server_sockets[index].request.get_method());
			server_sockets[index].send_action = (it != method_to_action.end()) ? it->second : SocketState::UNSUPPORTED;
			memcpy(server_sockets[index].buffer, &server_sockets[index].buffer[bytesRecv], server_sockets[index].length - bytesRecv);
			server_sockets[index].length -= bytesRecv;
		}
	}
}

void WebServer::send_message(const size_t index) {
	int bytesSent = 0;
	SOCKET msgSocket = server_sockets[index].id;
	
	do_action(index);
	bytesSent = send(msgSocket, send_buffer, (int)strlen(send_buffer), 0);
	if (SOCKET_ERROR == bytesSent) {
		std::cout << "Web Server: Error at send(): " << WSAGetLastError() << std::endl;	
		return;
	}

	std::cout << "Web Server: Sent: " << bytesSent << "\\" << strlen(send_buffer) << " bytes of \"" << send_buffer << "\" message.\n";	
	server_sockets[index].send = SocketState::IDLE;
}

void WebServer::do_action(const size_t index) {
	int action = server_sockets[index].send_action;
	const HTTPRequest& request = server_sockets[index].request;
	switch (action) {
	case SocketState::OPTIONS:
		do_options(request);
		break;
	case SocketState::GET:
		do_get(request);
		break;
	case SocketState::HEAD:
		do_head(request);
		break;
	case SocketState::POST:
		do_post(request);
		break;
	case SocketState::PUT:
		do_put(request);
		break;
	case SocketState::DEL:
		do_delete(request);
		break;
	case SocketState::TRACE:
		do_trace(request);
		break;
	default:
		unsupported_request(request);
		break;
	}
}

void WebServer::do_options(const HTTPRequest& request) {
	const char* ROOT_ALLOW = "OPTIONS, TRACE";
	const char* INDEX_ALLOW = "OPTIONS, GET, HEAD";
	const char* LIST_ALLOW = "OPTIONS, POST, DELETE";
	const char* DB_ALLOW = "OPTIONS, PUT, DELETE";
	HTTPResponse response;
	auto it = uri_to_resource.find(request.get_uri());
	if (it != uri_to_resource.end()) {
		response = HTTPResponse(204, request.get_http_version());
		switch (it->second) {
		case ROOT:
			response.insert_header("Allow", ROOT_ALLOW);
			break;
		case INDEX:
			response.insert_header("Allow", INDEX_ALLOW);
			break;
		case LIST:
			response.insert_header("Allow", LIST_ALLOW);
			break;
		case DB:
			response.insert_header("Allow", DB_ALLOW);
			break;
		case ASTERISK:
			break;
		}
		write_to_send_buffer(response);
	}
	else {
		resource_not_found(request);
	}
}
void WebServer::do_get(const HTTPRequest& request, const bool head_req) {
	const std::map<std::string, std::string> lang_to_filename = { {"en", "index-en.html"}, {"he", "index-he.html"}, {"fr", "index-fr.html"} };
	const std::string DEFAULT_LANG = "en";
	std::string lang_parameter;
	std::string index_filename;
	HTTPResponse response;
	auto uri_it = uri_to_resource.find(request.get_uri());
	if (uri_it != uri_to_resource.end()) {
		switch (uri_it->second) {
		case INDEX:
			response = HTTPResponse(200, request.get_http_version());
			if (request.get_queries().find("lang") != request.get_queries().end()) {
				lang_parameter = request.get_queries().at("lang");
				if (lang_to_filename.find(lang_parameter) != lang_to_filename.end()) {
					index_filename = lang_to_filename.at(lang_parameter);
				}
				else {
					index_filename = lang_to_filename.at(DEFAULT_LANG);
				}
			}
			else {
				index_filename = lang_to_filename.at(DEFAULT_LANG);
			}
			file_to_body(response, (RESOURCES_DIR + index_filename));
			if (response.get_status_code() != 500) {
				response.insert_header("Content-Type", "text/html");
			}
			if (head_req) {
				response.set_body();
			}
			write_to_send_buffer(response);
			break;
		default:
			resource_not_found(request);
			break;
		}
	}
	else {
		resource_not_found(request);
	}
}
void WebServer::do_head(const HTTPRequest& request) {
	do_get(request, true);
}
void WebServer::do_post(const HTTPRequest& request) {
	HTTPResponse response;
	std::string body;
	auto it = uri_to_resource.find(request.get_uri());
	if (it != uri_to_resource.end()) {
		switch (it->second) {
		case LIST:
			response = HTTPResponse(200, request.get_http_version());
			body = request.get_body();
			std::cout << "Adding to list: \"" << body << "\"\n";
			list.push_back(body);
			list_to_body(response);
			write_to_send_buffer(response);
			break;
		default:
			resource_not_found(request);
			break;
		}
	}
	else {
		resource_not_found(request);
	}
}
void WebServer::do_put(const HTTPRequest& request) {
	HTTPResponse response;
	std::string id;
	std::string value;
	auto it = uri_to_resource.find(request.get_uri());
	if (it != uri_to_resource.end()) {
		switch (it->second) {
		case DB:
			response = HTTPResponse(200, request.get_http_version());
			id = (request.get_queries().find("id") != request.get_queries().end()) ? request.get_queries().at("id") : std::to_string(db.size());
			value = request.get_body();
			std::cout << "Adding/Modifying to db: (id: " << id << ", value: \"" << value << "\")\n";
			if (db.find(id) == db.end()) { response.set_status_code(201); }
			db[id] = value;
			db_to_body(response);
			write_to_send_buffer(response);
			break;
		default:
			resource_not_found(request);
			break;
		}
	}
	else {
		resource_not_found(request);
	}
}
void WebServer::do_delete(const HTTPRequest& request) {
	HTTPResponse response;
	auto it = uri_to_resource.find(request.get_uri());
	if (it != uri_to_resource.end()) {
		switch (it->second) {
		case LIST:
			response = HTTPResponse(200, request.get_http_version());
			std::cout << "Clearing list..." << std::endl;
			list.clear();
			list_to_body(response);
			write_to_send_buffer(response);
			break;
		case DB:
			response.set_status_code(200);
			std::cout << "Clearing db..." << std::endl;
			db.clear();
			db_to_body(response);
			write_to_send_buffer(response);
			break;
		default:
			resource_not_found(request);
			break;
		}
	}
	else {
		resource_not_found(request);
	}
}
void WebServer::do_trace(const HTTPRequest& request) {
	HTTPResponse response;
	auto it = uri_to_resource.find(request.get_uri());
	if (it != uri_to_resource.end()) {
		switch (it->second) {
		case ROOT:
			response = HTTPResponse(200, request.get_http_version());
			response.insert_header("Content-Type", "message/http");
			response.set_body(request.get_message_copy());
			write_to_send_buffer(response);
			break;
		default:
			resource_not_found(request);
			break;
		}
	}
	else {
		resource_not_found(request);
	}
}
void WebServer::unsupported_request(const HTTPRequest& request) {
	HTTPResponse response(400, request.get_http_version());
	write_to_send_buffer(response);
}

void WebServer::resource_not_found(const HTTPRequest& request) {
	HTTPResponse response(404, request.get_http_version());
	write_to_send_buffer(response);
}

void WebServer::write_to_send_buffer(const HTTPResponse& response) {
	strcpy(send_buffer, response.write_response().c_str());
}

void WebServer::file_to_body(HTTPResponse& response, std::string file_name) {
	std::ifstream file_stream(file_name);
	if (!file_stream.is_open()) {
		response = HTTPResponse(500, response.get_http_version());
		return;
	}
	std::string line;
	while (std::getline(file_stream, line)) {
		response.append_to_body(line);
	}
	file_stream.close();
}

void WebServer::list_to_body(HTTPResponse& response) {
	constexpr size_t COMMA_SP = 2;
	response.append_to_body("{");
	for (const std::string& str : list) {
		response.append_to_body("\"");
		response.append_to_body(str);
		response.append_to_body("\"");
		response.append_to_body(", ");
	}
	if (response.get_body().size() >= 2) { response.pop_back_n_body(COMMA_SP); }
	response.append_to_body("}");
}

void WebServer::db_to_body(HTTPResponse& response) {
	constexpr size_t COMMA_SP = 2;
	response.append_to_body("{");
	for (const auto& pair : db) {
		response.append_to_body("(");
		response.append_to_body(pair.first);
		response.append_to_body(": ");
		response.append_to_body(pair.second);
		response.append_to_body(")");
		response.append_to_body(", ");
	}
	if (response.get_body().size() >= 2) { response.pop_back_n_body(COMMA_SP); }
	response.append_to_body("}");
}