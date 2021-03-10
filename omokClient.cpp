#include "headers/mySocket.h"
#include "headers/room.h"
#include "headers/clientDraw.h"
#include<vector>
#include<iomanip>
#include<sstream>
#include<future>
#include<chrono>
#include<atomic>

#ifdef _WIN32
#include <conio.h>
static HANDLE g_hScreen[2];
static int g_nScreenIndex;
#else
#include <termios.h>
#endif

int initPlayer();						// connect Server

void refreshMainUI();					// visualize UI

void getCommend();						// wait for Commend
void recvFromServer();					// wait for server's msg

void sendToServer(std::string);

void handleCommend(std::string commend);// handle commend


MySocket servSock(TCP);
std::string name;
std::string b;
std::string buffer;
std::vector<roomInfo> listData;
std::vector<std::string> chatData;

std::future<void> inputThread;
std::future<void> recvThread;

std::atomic_bool roomOpened(false);
std::atomic_bool serverConnected(false);
std::atomic_bool clntClose(false);
std::atomic_bool refreshLock(false);

std::recursive_mutex listLock;
std::recursive_mutex chatLock;

/*////////////////////////////////////
MAIN
/////////////////////////////////////*/

int main(void) {
	int commend;

	if (initPlayer() == -1) {
		chatData.push_back("connection fail");
	}
	
	inputThread = std::async(getCommend);
	recvThread = std::async(recvFromServer);	

	inputThread.get();
	recvThread.get();
	return 0;
}

/*////////////////////////////////////
CONNECT TO SERVER 
/////////////////////////////////////*/

int initPlayer() {
	if (servSock.connect("127.0.0.1", 9191)) {
		return -1;
	}
	serverConnected.exchange(true);

	std::cout << "input your name : ";
	std::cin >> name;

	if (servSock.send(name) == -1) {
		std::cout << "sendError" << std::endl;
		return -1;
	}
	std::string b = servSock.recv();
	b.pop_back();
	if (b != name) {
		std::cout << "recvError" << b << std::endl;
		return -1;
	}

	std::cout << "connected" << std::endl;
	return 0;
}

/*////////////////////////////////////
UI
/////////////////////////////////////*/

void refreshMainUI() {
	OmokdrawView drawer;

	std::lock_guard<std::recursive_mutex> vector_lock(chatLock);
	std::lock_guard<std::recursive_mutex> list_lock(listLock);

	if (!refreshLock) {
		drawer.drawRoomList(listData, 0);
		drawer.drawChat(chatData, 5);
		drawer.drawCommendLine(buffer);
		drawer.drawWindow();
	}
}


void handleCommend(std::string commend) {
	if (commend[0] != '/') {
		sendToServer(commend);
	}
	else {
		std::string a = commend.substr(1);
		if (a == "quit" || a == "q") {
			sendToServer("::end::");
			clntClose.exchange(true);
		}
		else if (a == "make" || a == "m") {
			std::string roomName, passwd;

			if (serverConnected == false)
				chatData.push_back("server Closed");

			screenClear();
			refreshLock.exchange(true);
			std::cout << "room name(1 ~ 15) : ";
			std::getline(std::cin, roomName);

			std::cout << "PassWords (Enter \"0\" to make public room) : ";
			std::getline(std::cin, passwd);

			refreshLock.exchange(false);

			sendToServer("::mkr::" + roomName + ":" + passwd);
		}
		else if (a == "access" || a == "a") {
			if (serverConnected == false)
				chatData.push_back("server Closed");
			
			int num;
			std::string pass;

			screenClear();
			refreshLock.exchange(true);

			std::cout << "room number : ";
			std::cin >> num;

			for (auto room : listData) {
				if (room.index == num) {
					if (room.isLocked)
						std::cout << "PassWords : ";
						std::cin >> pass;
						break;
				}
				else {
					pass = "0";
				}
			}

			refreshLock.exchange(false);

			sendToServer("::acr::" + std::to_string(num) + ":" + pass);
		}
		else if (a == "refresh" || a == "r") {
			if (serverConnected == false)
				chatData.push_back("server Closed");

			listData.clear();

			sendToServer("::rlt::");
		}
		else if (a == "help" || a == "h") {
			std::lock_guard<std::recursive_mutex> vector_lock(chatLock);
			chatData.push_back("/quit (/q): quit Omok");
			chatData.push_back("/make (/q): make room");
			chatData.push_back("/access (/q): access room with number");
			chatData.push_back("/refresh (/q): refresh room list");
		}
		else {
			std::lock_guard<std::recursive_mutex> vector_lock(chatLock);
			chatData.push_back("없는 명령어입니다.");
			chatData.push_back("/help를 눌러 명령어 목록을 확인하세요");
		}
	}
}

/*////////////////////////////////////
MUST CONNECT SERVER
/////////////////////////////////////*/

void sendToServer(std::string msg) {
	if (serverConnected == false)
		chatData.push_back("server Closed");

	servSock.send(msg);
}

/*////////////////////////////////////
MUST CONNECT TO SERVER
LOOP
/////////////////////////////////////*/


void recvFromServer() {
	std::string recvStr;
	std::string recvCMD;
	std::string recvOPT;
	while (clntClose == false) {
		while (serverConnected) {
			recvStr = servSock.recv();

			if (recvStr.size() <= 1 || recvStr.substr(0, 7) == "::end::") {
				serverConnected.exchange(false);
				chatData.push_back("Server Closed");
				break;
			}

			recvCMD = recvStr.substr(0, 7);
			recvOPT = recvStr.substr(7);

			if (recvStr.substr(0, 2) == "::") {
				recvOPT.pop_back();
				if (recvCMD == "::mkr::") {
					if (recvOPT == "OK")
						roomOpened.exchange(true);
				}
				else if (recvCMD == "::acr::") {
					if (recvOPT == "NOROOM") {
						chatData.push_back("NO ROOM");
					}
					else if (recvOPT == "PLAYING") {
						chatData.push_back("NOW PLAYING");
					}

				}
				else if (recvCMD == "::rlt::") {
					std::string name;
					int index;
					int status;
					bool isLocked;

					index = std::stoi(recvOPT.substr(0, recvOPT.find(':')));
					recvOPT = recvOPT.substr(recvOPT.find(':') + 1);

					status = std::stoi(recvOPT.substr(0, recvOPT.find(':')));
					recvOPT = recvOPT.substr(recvOPT.find(':') + 1);

					isLocked = std::stoi(recvOPT.substr(0, recvOPT.find(':')));
					recvOPT = recvOPT.substr(recvOPT.find(':') + 1);

					name = recvOPT;

					roomInfo temp(index, status, isLocked, name);
					{
						std::lock_guard<std::recursive_mutex> vector_lock(listLock);
						listData.push_back(temp);
					}
					refreshMainUI();
				}
			}
			else {
				std::lock_guard<std::recursive_mutex> vector_lock(chatLock);
				chatData.push_back(recvStr);
				refreshMainUI();
			}
		}
	}
}



/*////////////////////////////////////
LOOP
/////////////////////////////////////*/


void getCommend(){
	while (clntClose == false) {
		int input;
		input = _getch();

		switch (input)
		{
		case '\n':
		case '\r':
			if (buffer.size() != 0) {
				handleCommend(buffer);
				buffer.clear();
			}
			break;
		case 8:
			if(buffer.size() != 0)
				buffer.pop_back();
			break;

		default:
			buffer.push_back(input);
			break;
		}
		refreshMainUI();
	}
}

