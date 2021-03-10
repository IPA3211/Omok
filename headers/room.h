#pragma once
#include <string>
#include "mySocket.h"

#define ROOM_STATUS_IDLE 0
#define ROOM_STATUS_PLAY 1
#define ROOM_STATUS_ERR -1

#define ALLDATA 0
#define ADDRDATA 1
#define NAMEDATA 2

struct roomInfo
{
	int index;
	int status;
	bool isLocked;
	std::string name;

	roomInfo(int i, int s, bool b, std::string n) {
		index = i;
		status = s;
		isLocked = b;
		name = n;
	}
};

class room {
public:
	void init(std::string ip, short port,std::string name, int status, std::string passwd);
	void init(std::string data);

	room();
	room(std::string ip, short port, std::string name, int status, std::string passwd);
	room(std::string data);
		
	room& operator=(const room &other);

	void setStatus(int s);
	void setName(std::string);
	void setPasswd(std::string);

	int getStatus() { return _status; }
	std::string getName() { return _name; }
	std::string getPasswd() { return _passwd; }

	std::string serializeData(int);

private:
	std::string _ip;
	unsigned short _port;
	std::string _name;
	int _status;
	std::string _passwd;
};

void room::init(std::string ip, short port, std::string name, int status, std::string passwd) {
	_ip = ip;
	_port = port;
	_name = name;
	_status = status;
	_passwd = passwd;
}

void room::init(std::string data) {
	//data stream ex) ip(addr_s):name:status:passwd
	
	std::string ip;
	short port;
	std::string name;
	int status;
	std::string passwd;
	
	ip = data.substr(0, data.find(':'));
	data = data.substr(data.find(':') + 1);

	port = std::stoi(data.substr(0, data.find(':')));
	data = data.substr(data.find(':') + 1);

	name = data.substr(0, data.find(':'));
	data = data.substr(data.find(':') + 1);

	status = std::stoi(data.substr(0, data.find(':')));
	data = data.substr(data.find(':') + 1);

	passwd = data;

	init(ip, port, name, status, passwd);
}

room::room() {
	init("", 0, "", ROOM_STATUS_ERR, "");
}

room::room(std::string ip, short port, std::string name, int status, std::string passwd) {
	init(ip, 0, name, status, passwd);
}

room::room(std::string data) {
	init(data);
}

room& room::operator=(const room &other) {
	if (this == &other)
		return *this;

	init(other._ip, other._port, other._name, other._status, other._passwd);

	return *this;
}

void room::setName(std::string name) {	_name = name;}
void room::setPasswd(std::string ps) { _passwd = ps; }
void room::setStatus(int s) { _status = s; }

std::string room::serializeData(int type) {
	int isLocked;

	switch (type)
	{
	case ADDRDATA :
		return _ip + ":" + std::to_string(_port);

	case ALLDATA:
		isLocked = (0 != _passwd.size()) ? 1 : 0;

		return _ip + ":"
			+ std::to_string(_port) + " : "
			+ _name + ":"
			+ std::to_string(_status) + ":"
			+ std::to_string(isLocked);

	case NAMEDATA:
		isLocked = (0 != _passwd.size()) ? 1 : 0;

		return 
			std::to_string(_status) + ":"
			+ std::to_string(isLocked) + ":"
			+_name;
	default:
		break;
	}
}