#pragma once
#include"room.h"
#include<vector>
#include<iostream>
#include<iomanip>
#include<sstream>

std::string strCenter(std::string, int);
void screenClear();

void screenClear() {
#ifdef _WIN32
	unsigned long dw;
	COORD pos = { 0, 0 };
	SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), pos);
	FillConsoleOutputCharacter(GetStdHandle(STD_OUTPUT_HANDLE), ' ', 300 * 300, { 0, 0 }, &dw);
#else
	system("clear");
#endif
}

std::string strCenter(std::string msg, int size) {
	std::string a = msg;

	for (; a.size() < size;) {
		a = " " + a + " ";
	}

	if (a.size() > size) {
		a = a.substr(0, size);
	}

	return a;
}

class OmokdrawView
{
public:
	OmokdrawView();
	OmokdrawView(int index, int name, int status, int lock);
	~OmokdrawView();
	void init(int index, int name, int status, int lock);
	void drawRoomList(std::vector<roomInfo>, int);
	void drawChat(std::vector<std::string>, int);
	void drawCommendLine(std::string);
	void drawWindow();

private:
	std::stringstream ss;
	int drawIndex;
	int drawName;
	int drawStatus;
	int drawLock;
};
void OmokdrawView::init(int index, int name, int status, int lock) {
	drawIndex = index;
	drawName = name;
	drawStatus = status;
	drawLock = lock;
}

OmokdrawView::OmokdrawView()
{
	init(5, 30, 10, 5);
}

OmokdrawView::OmokdrawView(int index, int name, int status, int lock)
{
	init(index, name, status, lock);
}

OmokdrawView::~OmokdrawView()
{
}

void OmokdrawView::drawRoomList(std::vector<roomInfo> list, int startPoint = 0) {
	int i;

	for (int a = 0; a < drawIndex + drawName + drawStatus + drawLock + 5; a++)
		std::cout << "-";
	std::cout << std::endl;

	ss << "|";
	ss << std::setw(drawIndex) << std::left << "Index";
	ss << "|";
	ss << std::setw(drawName) << strCenter("Room name", drawName);
	ss << "|";
	ss << std::setw(drawStatus) << strCenter("Status", drawStatus);
	ss << "|";
	ss << std::setw(drawLock) << std::left << "Lock";
	ss << "|";
	ss << std::endl;

	for (int a = 0; a < drawIndex + drawName + drawStatus + drawLock + 5; a++)
		ss << "-";

	ss << std::endl;

	for (i = startPoint; i < list.size() - startPoint; i++) {
		roomInfo r = list[i];
		ss << "|";
		ss << std::setw(drawIndex) << std::left << r.index;
		ss << "|";
		ss << std::setw(drawName) << strCenter(r.name, drawName);
		ss << "|";
		ss << std::setw(drawStatus) << strCenter(r.status == ROOM_STATUS_IDLE ? "Empty" : "Playing", drawStatus);
		ss << "|";
		ss << std::setw(drawLock) << std::left << strCenter(r.isLocked ? "Y" : "N", drawLock);
		ss << "|";
		ss << std::endl;
	}

	for (int j = 0; j < 50 - i; j++) {
		ss << "|";
		ss << std::setw(drawIndex) << " ";
		ss << "|";
		ss << std::setw(drawName) << " ";
		ss << "|";
		ss << std::setw(drawStatus) << " ";
		ss << "|";
		ss << std::setw(drawLock) << " ";
		ss << "|";
		ss << std::endl;
	}

	for (int a = 0; a < drawIndex + drawName + drawStatus + drawLock + 5; a++)
		ss << "-";

	ss << std::endl;
}

void OmokdrawView::drawChat(std::vector<std::string> chat, int lines = 5) {
	for (int i = 0; i < lines && i < chat.size(); i++) {
		if(chat.size() > lines)
			ss << chat[chat.size() - lines + i] << std::endl;
		else{
			ss << chat[i] << std::endl;
		}
	}
	ss << std::endl;
}

void OmokdrawView::drawCommendLine(std::string commend) {
	ss << "Input : " << commend;
}

void OmokdrawView::drawWindow() {
	screenClear();

	std::string a;
	while (std::getline(ss, a)) {
		printf("%s\n", a);
	}

	ss.clear();
}
