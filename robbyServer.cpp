#include "headers/mySocket.h"
#include "headers/room.h"
#include <future>
#include <atomic>
#include <array>
#include <mutex>

#define MAX_PLAYER 200

struct Client {
	bool _isConnected = false;
	std::shared_ptr<MySocket> _sock;
	std::string _name;

	Client(bool a, std::shared_ptr<MySocket> b) {
		_isConnected = a;
		_sock = b;
	}

	Client() {
		_isConnected = false;
		_sock = std::make_shared<MySocket>(TCP);
		_name = "";
	}
};

struct roomClient {
	std::shared_ptr<Client> _client;
	room _roomData;

	roomClient(std::shared_ptr<Client> client, room r) {
		_client = client;
		_roomData = r;
	}

	roomClient() {
		_client = NULL;
	}

};

void waitServerCommand();

void threadHendler();

void startServer();

void acceptClient();
void acceptClientLoop();

void closeSquence();
void serveClient(int);

std::atomic<bool> isRunning(false);

MySocket sock(TCP);
std::array<roomClient, MAX_PLAYER> roomClients;
std::array<Client, MAX_PLAYER> clients;
std::array<std::future<void>, MAX_PLAYER> threads;

std::recursive_mutex socketVectorLock;
std::recursive_mutex socketThreadLock;

std::future<void> commendThread;
std::future<void> acceptThread;
std::future<void> handlerThread;

int main()
{
	unsigned short port;
	
	std::cout << "input server port number : ";
	std::cin >> port;
	
	sock.bind(port);
	sock.listen(512);

	//commendThread = std::async(waitServerCommand);
	acceptThread = std::async(acceptClientLoop);

	waitServerCommand();

	closeSquence();

	acceptThread.get();
	//commendThread.get();
	return 0;
}

void threadHendler() {
	for (int i = 0; i <= MAX_PLAYER; i++) {
		if (isRunning.load(std::memory_order_relaxed) == false) {
			break;
		}
		if (MAX_PLAYER == i) {
			i = 0;
		}
		if (threads[i].valid() == false) {
			continue;
		}
		auto status = threads[i].wait_for(std::chrono::microseconds(1));
		if (status == std::future_status::ready) {
			std::cout << i << " : Thread finished" << std::endl;
			threads[i].get();
		}
	}
}

void closeSquence() {
	for (int i = 0; i < MAX_PLAYER; i++) {
		if (threads[i].valid() == false) {
			continue;
		}
		auto status = threads[i].wait_for(std::chrono::seconds(5));
		if (status == std::future_status::ready) {
			std::cout << i << " : Thread finished" << std::endl;
			threads[i].get();
		}
		else {
			std::cout << "W : " << i << " : Thread didn't close" << std::endl;
		}
	}

	sock.close();
}

void waitServerCommand() {
	std::string commend;
	while (1) {
		std::cout << "INPUT : ";
		std::cin >> commend;

		std::cout << commend;

		if (commend == "q") {
			isRunning.store(true, std::memory_order_relaxed);
			break;
		}
	}
}

void acceptClient(){
	int i = 0;
	std::shared_ptr<MySocket> temp(new MySocket(TCP));
	MySocket clnt = sock.accept();
	*temp = clnt;
	std::cout << "something accepted" << std::endl;
	{
		std::lock_guard<std::recursive_mutex> vector_lock(socketVectorLock);
		for (i = 0; i < MAX_PLAYER; i++) {
			if (clients[i]._isConnected == false) {
				clients[i]._isConnected = true;
				clients[i]._sock = temp;
				break;
			}
		}
	}
	std::cout << "something happend" << std::endl;
	{
		std::lock_guard<std::recursive_mutex> thread_lock(socketThreadLock);
		threads[i] = std::async(serveClient, i);
	}
}

void acceptClientLoop() {
	std::cout << "Server START" << std::endl;
	isRunning.store(true, std::memory_order_relaxed);

	handlerThread = std::async(threadHendler);

	while (isRunning.load(std::memory_order_relaxed)) {
		acceptClient();
	}
	std::cout << "Server STOP" << std::endl;
}

void serveClient(int index) {
	std::string buffer;
	std::string name;
	std::cout << "in" << std::endl;

	bool isConnected;
	MySocket clnt(TCP);
	{
		std::lock_guard<std::recursive_mutex> vector_lock(socketVectorLock);
		isConnected = clients[index]._isConnected;
		clnt = *clients[index]._sock;
	}

	name = clnt.recv();
	name.pop_back();

	if (name.size() <= 1) {
		std::lock_guard<std::recursive_mutex> vector_lock(socketVectorLock);
		clients[index]._isConnected = false;
		clients[index]._sock = NULL;
		clnt.close();
		return;
	}

	{
		std::lock_guard<std::recursive_mutex> vector_lock(socketVectorLock);
		clients[index]._name = name;
	}

	clnt.send(name);

	while (1) {

		if (isRunning.load(std::memory_order_relaxed) == false)
			break;

		buffer = clnt.recv();

		// when client shutdown
		if (buffer.size() <= 1 || buffer.substr(0, 7) == "::end::") {
			std::lock_guard<std::recursive_mutex> vector_lock(socketVectorLock);
			clients[index]._isConnected = false;
			clients[index]._sock = NULL;
			clients[index]._name.clear();

			for (int i = 0; i < MAX_PLAYER; i++) {
				if (clients[i]._isConnected)
					clients[i]._sock->send(name + " is out");
			}
			break;
		}

		//when client want to make a room ::mkr::name:passwd
		else if (buffer.substr(0, 7) == "::mkr::") {
			for (int i = 0; i < MAX_PLAYER; i++) {
				if (roomClients[i]._roomData.getStatus() == ROOM_STATUS_ERR) {
					std::lock_guard<std::recursive_mutex> vector_lock(socketVectorLock);
					roomClients[i]._roomData.init(inet_ntoa(clnt.getAddr().sin_addr), clnt.getAddr().sin_port,
													buffer.substr(7, buffer.find(':')), ROOM_STATUS_IDLE, buffer.substr(buffer.find(':') + 1));
					clnt.send("OK");
				}
			}
		}
		//when client want to access a room
		//client send ex) "::acr::7:979"
		else if (buffer.substr(0, 7) == "::acr::") {
			std::string temp = buffer.substr(7);
			int roomNum = std::stoi(temp.substr(0, temp.find(':')));
			temp = temp.substr(temp.find(':') + 1);

			for (int i = 0; i < MAX_PLAYER; i++) {
				switch (roomClients[roomNum]._roomData.getStatus())
				{
				case ROOM_STATUS_IDLE:
					if(roomClients[roomNum]._roomData.getPasswd() == temp)
					clnt.send(roomClients[roomNum]._roomData.serializeData(ADDRDATA));
					break;
				case ROOM_STATUS_PLAY:
					clnt.send("PLAYING");
					break;
				case ROOM_STATUS_ERR:
					clnt.send("NOROOM");
					break;
				default:
					clnt.send("NOROOM");
					break;
				}
			}
		}
		//when client want room list
		else if (buffer.substr(0, 7) == "::rlt::") {
			for (int i = 0; i < MAX_PLAYER; i++) {
				switch (roomClients[std::stoi(buffer.substr(7))]._roomData.getStatus())
				{
				case ROOM_STATUS_IDLE:
					clnt.send(roomClients[std::stoi(buffer.substr(7))]._roomData.serializeData(ALLDATA));
					break;
				case ROOM_STATUS_PLAY:
					clnt.send(roomClients[std::stoi(buffer.substr(7))]._roomData.serializeData(ALLDATA));
					break;
				case ROOM_STATUS_ERR:
					break;
				default:
					break;
				}
			}

			clnt.send("LTEND");
		}

		//when client want other client list

		//send message to every clients
		else
		{
			std::lock_guard<std::recursive_mutex> vector_lock(socketVectorLock);
			for (int i = 0; i < MAX_PLAYER; i++) {
				if (clients[i]._isConnected)
					if (clients[i]._sock->send("" + name + " : " + buffer) == -1) {
						clients[index]._isConnected = false;
						clients[index]._sock = NULL;
						clients[index]._name.clear();

						for (int i = 0; i < MAX_PLAYER; i++) {
							if (clients[i]._isConnected)
								clients[i]._sock->send(name + " is out");
						}
						break;
					}
			}
		}
	}

	clnt.close();

	std::cout << index << " good bye!" << std::endl;
}