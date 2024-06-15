#pragma once
class ServerException {};
class InitWinSockException : public ServerException {};
class CreateListenerSocketException : public ServerException {};
class BindListenerSocketException : public ServerException {};
class StartListenerException : public ServerException {};
class SelectException : public ServerException {};