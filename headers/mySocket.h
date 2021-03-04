#pragma once

#include <iostream>
#include <string>

#ifdef __linux__

#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define SOCKET int
#define SOCKET_ERROR -1

#endif


#ifdef _WIN32
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#pragma comment(lib, "ws2_32.lib")
#include <WinSock2.h>
#define socklen_t int

#endif

#define TCP 0
#define UDP 1

#define BUFFER_SIZE 1024

class SockAddrSet
{
public:
	sockaddr_in _addr;
	socklen_t _size;

	SockAddrSet(const std::string &ip, short port);
	SockAddrSet(short port);
	SockAddrSet();
	~SockAddrSet();
};

SockAddrSet::SockAddrSet(const std::string &ip, short port)
{
	_addr.sin_family = AF_INET;
	_addr.sin_addr.s_addr = inet_addr(ip.c_str());
	_addr.sin_port = htons(port);

	_size = sizeof(_addr);
}

SockAddrSet::SockAddrSet(short port) {
	_addr.sin_family = AF_INET;
	_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	_addr.sin_port = htons(port);

	_size = sizeof(_addr);
}

SockAddrSet::SockAddrSet() { _size = sizeof(_addr); }

SockAddrSet::~SockAddrSet()
{

}

class MySocket
{
public:
	bool debugMode = true;

	MySocket();
	MySocket(const int &type);
	MySocket(const MySocket &other);
	MySocket(int sock, const int &type, sockaddr_in addr);

	MySocket& operator=(MySocket& ref);

	int bind(const short &port);
	int connect(std::string ip, const short &port);
	int listen(int waiting);
	MySocket accept();

	int send(std::string msg);
	int sendTo(std::string msg, const SockAddrSet& to);
	std::string recv();
	std::string recvFrom(SockAddrSet& from);

	int close();

	void getSocketInfo(int& _sock, int& _port, int& _type, sockaddr_in& addr);
	sockaddr_in getAddr() { return _addr; }

	static void errorHandler(std::string message);

	~MySocket();

private:
#ifdef _WIN32
	WSADATA WsaData;
#endif
	int _type = 0;
	SOCKET _sock = 0;
	short _port = 0;
	std::string _ip = "";
	sockaddr_in _addr = {};

	bool isConnected = false;
};

MySocket::MySocket() {
#ifdef _WIN32
	if (WSAStartup(MAKEWORD(2, 2), &WsaData) != 0)
		errorHandler("SetUp error");
#endif
}

MySocket::MySocket(const int &type)
{
#ifdef _WIN32
	if (WSAStartup(MAKEWORD(2, 2), &WsaData) != 0)
		errorHandler("SetUp error");
#endif
	_type = type;
	switch (type)
	{
	case TCP:
		_sock = socket(PF_INET, SOCK_STREAM, 0);
		break;
	case UDP:
		_sock = socket(PF_INET, SOCK_DGRAM, 0);
	default:
		break;
	}
}

MySocket::MySocket(const MySocket &other) {
#ifdef _WIN32
	if (WSAStartup(MAKEWORD(2, 2), &WsaData) != 0)
		errorHandler("SetUp error");
#endif
	_type = other._type;
	_sock = other._sock;
	_port = other._port;
	_ip = other._ip;
	_addr = other._addr;
}

MySocket::MySocket(int sock, const int &type, sockaddr_in addr) {
#ifdef _WIN32
	if (WSAStartup(MAKEWORD(2, 2), &WsaData) != 0)
		errorHandler("SetUp error");
#endif
	_sock = sock;
	_type = type;
	_addr = addr;
}

MySocket& MySocket::operator=(MySocket& ref) {
#ifdef _WIN32
	if (WSAStartup(MAKEWORD(2, 2), &WsaData) != 0)
		errorHandler("SetUp error");
#endif
	_type = ref._type;
	_sock = ref._sock;
	_port = ref._port;
	_ip = ref._ip;
	_addr = ref._addr;
	return *this;
}

int MySocket::bind(const short &port) {
	_port = port;
	_addr.sin_family = AF_INET;
	_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	_addr.sin_port = htons(_port);

	int temp = ::bind(_sock, (sockaddr *)&_addr, sizeof(_addr));

	if (debugMode) {
		if (temp == SOCKET_ERROR) {
			this->close();
			errorHandler("bind() error");
		}
	}

	return temp;
}

int MySocket::connect(std::string address, const short &port) {
	_addr.sin_family = AF_INET;
	_addr.sin_addr.s_addr = inet_addr(address.c_str());
	_addr.sin_port = htons(port);

	int temp = ::connect(_sock, (struct sockaddr*)&_addr, sizeof(_addr));

	if (debugMode) {
		if (temp == -1) {
			errorHandler("connect() error");
		}
	}

	if (temp != -1)
		isConnected = true;

	return temp;
}

int MySocket::listen(int wating) {
	int temp = ::listen(_sock, wating);

	if (debugMode) {
		if (temp == -1) {
			errorHandler("listen() error");
		}
	}

	return temp;
}

MySocket MySocket::accept() {
	sockaddr_in clntAddr = {};
	socklen_t clntAddrSize;

	clntAddrSize = sizeof(clntAddr);

	int clntSock = ::accept(_sock, (sockaddr *)&clntAddr, &clntAddrSize);

	if (debugMode) {
		if (clntSock == -1) {
			errorHandler("accept() error");
		}
	}

	MySocket tempSock(clntSock, _type, clntAddr);

	return tempSock;
}

int MySocket::send(std::string msg) {
	std::string buffer;
	int bfSize = BUFFER_SIZE - 2;
	switch (_type)
	{
	case TCP:
#ifdef _WIN32
		return ::send(_sock, msg.c_str(), msg.size(), 0);
#else
		return write(_sock, msg.c_str(), msg.size());
#endif
		break;
	case UDP:
		for (int i = 0; ; i++) {
			if (msg.size() > bfSize * (i + 1))
				buffer = msg.substr(bfSize * i, bfSize);
			else
				buffer = msg.substr(bfSize * i, msg.size() - bfSize * i);

#ifdef _WIN32
			int a = ::send(_sock, msg.c_str(), msg.size(), 0);
#else
			int a = write(_sock, msg.c_str(), msg.size());
#endif

			std::cout << a << std::endl;
			if (a <= bfSize - 1)
				return a;
		}
		break;
	default:
		break;
	}

	return 0;
}

int MySocket::sendTo(std::string msg, const SockAddrSet& to) {
	return ::sendto(_sock, msg.c_str(), msg.size(), 0,
		(struct sockaddr*)&to, sizeof(to));
}

std::string MySocket::recv() {
	char readBuffer[BUFFER_SIZE];
	int recvSize;
	std::string data;
	while (1) {
#ifdef _WIN32
		recvSize = ::recv(_sock, readBuffer, BUFFER_SIZE - 1, 0);
#else
		recvSize = read(_sock, readBuffer, BUFFER_SIZE - 1);
#endif

		if (recvSize == -1) {
			return "";
		}

		readBuffer[recvSize] = 0;

		data = data + readBuffer;

		if (recvSize < BUFFER_SIZE - 1) {
			break;
		}
	}

	data.push_back(0);

	return data;
}

std::string MySocket::recvFrom(SockAddrSet& from) {
	char readBuffer[BUFFER_SIZE];
	int recvSize;
	std::string data;
	while (1) {
		recvSize = ::recvfrom(_sock, readBuffer, BUFFER_SIZE - 1, 0,
			(struct sockaddr *)&from._addr, &from._size);

		readBuffer[recvSize] = 0;

		data = data + readBuffer;

		if (recvSize < BUFFER_SIZE - 1) {
			break;
		}
	}

	data.push_back(0);

	return data;
}

int MySocket::close() {
#ifdef __linux__
	::close(_sock);
#endif
#ifdef _WIN32
	::closesocket(_sock);
	WSACleanup();
#endif
	return 0;
}

MySocket::~MySocket()
{
}

void MySocket::errorHandler(std::string message) {
	std::cout << message << std::endl;
	exit(1);
}
