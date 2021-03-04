#include "headers/mySocket.h"
#include "headers/room.h"
#include<vector>
#include<iomanip>
#include<sstream>
#include<future>
#include<chrono>

#ifdef _WIN32
#include <conio.h>
static HANDLE g_hScreen[2];
static int g_nScreenIndex;
#else
#include <termios.h>
#endif

#define DRAW_INDEX 5
#define DRAW_NAME 50
#define DRAW_STATUS 10
#define DRAW_LOCK 5

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

int initPlayer();

void drowRoomList(std::vector<roomInfo>, int);
void drowChat(std::vector<std::string>, int);
void drowCommendLine(std::string);
void refreshMainUI();

void getCommend();
void handleCommend(std::string commend);

std::string strCenter(std::string, int);
void errorHandler(std::string);

MySocket servSock(TCP);
std::string name;
std::string buffer;
std::vector<roomInfo> listData;
std::vector<std::string> chatData;

std::future<void> inputThread;

std::stringstream ss;

int main(void) {
	int commend;

//	if (initPlayer() == -1) {
//		errorHandler("Connection Error");
//	}

	inputThread = std::async(getCommend);

	auto s = inputThread.wait_for(std::chrono::microseconds(1));
	while (true) {
		if (s == std::future_status::ready) {
			inputThread.get();
			break;
		}
	}
	return 0;
}

int initPlayer() {
	if (servSock.connect("127.0.0.1", 9191)) {
		return -1;
	}

	std::cout << "input your name : ";
	std::cin >> name;

	if (servSock.send(name) == -1) {
		std::cout << "sendError" << std::endl;
		return -1;
	}

	if (servSock.recv() != name) {
		std::cout << "recvError" << std::endl;
		return -1;
	}

	std::cout << "connected" << std::endl;
	return 0;
}

void drowRoomList(std::vector<roomInfo> list, int startPoint = 0) {
	int i;

	for (int a = 0; a < DRAW_INDEX + DRAW_NAME + DRAW_STATUS + DRAW_LOCK + 5; a++)
		std::cout << "-";
	std::cout << std::endl;

	ss << "|";
	ss << std::setw(DRAW_INDEX) << std::left << "Index";
	ss << "|";
	ss << std::setw(DRAW_NAME) << strCenter("Room name", DRAW_NAME);
	ss << "|";
	ss << std::setw(DRAW_STATUS) << strCenter("Status", DRAW_STATUS);
	ss << "|";
	ss << std::setw(DRAW_LOCK) << std::left << "Lock";
	ss << "|";
	ss << std::endl;

	for (int a = 0; a < DRAW_INDEX + DRAW_NAME + DRAW_STATUS + DRAW_LOCK + 5; a++)
		ss << "-";

	ss << std::endl;

	for (i = startPoint; i < list.size() - startPoint; i++) {
		roomInfo r = list[i];
		ss << "|";
		ss << std::setw(DRAW_INDEX) << std::left << r.index;
		ss << "|";
		ss << std::setw(DRAW_NAME) << strCenter(r.name, DRAW_NAME);
		ss << "|";
		ss << std::setw(DRAW_STATUS) << strCenter(r.status == ROOM_STATUS_IDLE ? "Empty" : "Playing", DRAW_STATUS);
		ss << "|";
		ss << std::setw(DRAW_LOCK) << std::left << strCenter(r.isLocked ? "Y" : "N", DRAW_LOCK);
		ss << "|";
		ss << std::endl;
	}

	for (int j = 0; j < 50 - i; j++) {
		ss << "|";
		ss << std::setw(DRAW_INDEX) << " ";
		ss << "|";
		ss << std::setw(DRAW_NAME) << " ";
		ss << "|";
		ss << std::setw(DRAW_STATUS) << " ";
		ss << "|";
		ss << std::setw(DRAW_LOCK) << " ";
		ss << "|";
		ss << std::endl;
	}

	for (int a = 0; a < DRAW_INDEX + DRAW_NAME + DRAW_STATUS + DRAW_LOCK + 5; a++)
		ss << "-";
}

void drowChat(std::vector<std::string> chat, int lines = 5) {
	for (int i = 0; i < lines && i < chat.size(); i++) {
		ss << chatData[chatData.size() - 1 + i] << std::endl;
	}
}

void drowCommendLine(std::string commend) {
	ss << "Input : " << commend;
}

void refreshMainUI() {

#ifdef _WIN32
	unsigned long dw;
	COORD pos = { 0, 0 };
	SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), pos);
	FillConsoleOutputCharacter(GetStdHandle(STD_OUTPUT_HANDLE), ' ', 300 * 300, { 0, 0 }, &dw);
#else
	system("clear");
#endif

	drowRoomList(listData, 0);
	ss << std::endl;
	drowChat(chatData, 5);
	ss << std::endl;
	drowCommendLine(buffer);

	std::string a;
	while (std::getline(ss, a)) {
		printf("%s\n", a);
	}

	ss.clear();

}

void getCommend(){
	while (true) {
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

void handleCommend(std::string commend) {
	if (commend[0] != '/') {
		servSock.send(commend);
	}
	else {
		std::string a = commend.substr(1);
		if (commend == "quit" || commend == "q") {
			servSock.send("::end::");
		}
		else if (commend == "make" || commend == "m") {
			servSock.send("::mkr::");
		}
		else if (commend == "access" || commend == "a") {
			servSock.send("::acr::");
		}
		else if (commend == "refresh" || commend == "r") {
			servSock.send("::rlt::");
		}
		else if (commend == "help" || commend == "h") {
			chatData.push_back("/quit (/q): quit Omok");
			chatData.push_back("/make (/q): make room");
			chatData.push_back("/access (/q): access room with number");
			chatData.push_back("/refresh (/q): refresh room list");
		}
		else {
			chatData.push_back("없는 명령어입니다.");
			chatData.push_back("/help를 눌러 명령어 목록을 확인하세요");
		}
	}
}

std::string strCenter(std::string msg, int size) {
	std::string a = msg;

	for (;a.size() < size;) {
		a = " " + a + " ";
	}

	if (a.size() > size) {
		a = a.substr(0, size);
	}

	return a;
}

void errorHandler(std::string msg) {
	std::cout << msg << std::endl;
	exit(0);
}