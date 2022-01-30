#include "Game.h"
#include "Socket.h"

#include <cstdlib>
#include <cstdio>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <error.h>
#include <netdb.h>
#include <sys/epoll.h>
#include <poll.h> 
#include <thread>
#include <unordered_set>
#include <signal.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <string>
#include <cstring>
#include <random>
#include <map>
#include <stdexcept>

// server socket
int servFd;

// client sockets
std::unordered_set<int> clientFds;

// handles SIGINT
void ctrl_c(int);

// converts cstring to port
uint16_t readPort(char * txt);

// sets SO_REUSEADDR
void setReuseAddr(int sock);

void readData();

int generateGameId();

void handleMessage(std::string mess, Socket* sock);

bool isIdExists(int id);

int epoll_fd;
std::map <int, Game*> games = {};


int main(){
	epoll_fd = epoll_create1(0);

	if(epoll_fd == -1){
		fprintf(stderr, "Failed to create epoll descriptor\n");
	}

	// get and validate port number
	char portNumber[] = "5050";
	auto port = readPort(portNumber);
	
	// create socket
	servFd = socket(AF_INET, SOCK_STREAM, 0);
	if(servFd == -1) error(1, errno, "socket failed");
	
	// graceful ctrl+c exit
	signal(SIGINT, ctrl_c);
	// prevent dead sockets from throwing pipe errors on write
	signal(SIGPIPE, SIG_IGN);
	
	setReuseAddr(servFd);

	epoll_event event;
	event.events = EPOLLIN;
	
	// bind to any address and port provided in arguments
	sockaddr_in serverAddr{.sin_family=AF_INET, .sin_port=htons((short)port), .sin_addr={INADDR_ANY}};
	int res = bind(servFd, (sockaddr*) &serverAddr, sizeof(serverAddr));
	if(res) error(1, errno, "bind failed");
	
	// enter listening mode
	res = listen(servFd, 1);
	if(res) error(1, errno, "listen failed");
	
/****************************/

	std::thread t1(readData);
	
	while(true){
		// prepare placeholders for client address
		sockaddr_in clientAddr{0};
		socklen_t clientAddrSize = sizeof(clientAddr);
		
		// accept new connection
		auto clientFd = accept(servFd, (sockaddr*) &clientAddr, &clientAddrSize);
		if(clientFd == -1) error(1, errno, "accept failed");

		// fcntl(clientFd, F_SETFL, fcntl(clientFd, F_GETFL) | O_NONBLOCK); 
		fcntl(clientFd, F_SETFL, O_NONBLOCK, 1);

		// connection_t* connection = new connection_t;
		Socket* socket = new Socket();
		socket->sock = clientFd;
		event.data.ptr = socket;
		
		epoll_ctl(epoll_fd, EPOLL_CTL_ADD, clientFd, &event);

		printf("new connection from: %s:%hu (fd: %d)\n", inet_ntoa(clientAddr.sin_addr), ntohs(clientAddr.sin_port), clientFd);

	}

	t1.join();
	
}

uint16_t readPort(char * txt){
	char * ptr;
	auto port = strtol(txt, &ptr, 10);
	if(*ptr!=0 || port<1 || (port>((1<<16)-1))) error(1,0,"illegal argument %s", txt);
	return port;
}

void setReuseAddr(int sock){
	const int one = 1;
	int res = setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));
	if(res) error(1,errno, "setsockopt failed");
}

void ctrl_c(int){
	for(int clientFd : clientFds)
		close(clientFd);
	close(servFd);
	printf("Closing server\n");
	exit(0);
}

void readData(){
	epoll_event event;
	Socket* sock;
	
	while(true){
		epoll_wait(epoll_fd, &event, 1, -1);
		if(event.events & EPOLLIN){
			
			sock = (Socket*)event.data.ptr;
			try{
				sock->readData();

				for(const auto &i : sock->message){
					handleMessage(i, sock);
				}
				sock->message.clear();
			}
			catch(const char* e){
				printf("exception: %s\n", e);
				epoll_ctl(epoll_fd, EPOLL_CTL_DEL, sock->sock, NULL);
				sock->socketClose();
				if(!std::strcmp(e, "Bad id") && !std::strcmp(e, "Bad user") && !std::strcmp(e, "Game already started")){
					if(sock->game->isHost(sock->sock)){
						sock->game->onHostDisconnected();
						games.erase(sock->game->getGameId());
						delete sock->game;
					}
					else{
						sock->game->onPlayerDisconnected(sock->sock);
					}
				}
				delete sock;
			}
		}
	}
}

int generateGameId(){
	std::random_device rd;
	std::mt19937 gen(rd());
	std::uniform_int_distribution<> distr(100000, 999999);
	return distr(gen);
}

void handleMessage(std::string strMess, Socket* sock){
	size_t foundName = strMess.find("\\create_game\\");
	size_t foundSendAnswers = strMess.find("\\send_answers\\");
	size_t foundJoin = strMess.find("\\join_game\\");
	size_t foundStart = strMess.find("\\start_game\\");
	size_t foundNext = strMess.find("\\next_question\\");
	size_t foundStudentAnswer = strMess.find("\\answer_student\\");
	size_t foundEndGame = strMess.find("\\end_game\\");
	size_t foundCountdown = strMess.find("\\countdown_end\\");
	if(foundName == 0){
		size_t foundQuantity = strMess.find("\\quantity\\");
		size_t foundTime = strMess.find("\\time\\");
		std::string gameName = std::string(&strMess[13], &strMess[foundQuantity]);
		std::string gameQuantity = std::string(&strMess[foundQuantity+10], &strMess[foundTime]);
		std::string gameTime = strMess.substr(foundTime+6);
		int gameId = generateGameId();
		while(isIdExists(gameId)){
			gameId = generateGameId();
		}
		Game* myGame = new Game(gameName, gameId, std::stoi(gameQuantity), std::stoi(gameTime), sock);
		sock->game = myGame;
		sock->writeData(std::to_string(gameId));
		games.insert({gameId, myGame});
	}
	else if(foundSendAnswers == 0){
		size_t foundAnswers = strMess.find("\\answers\\");
		bool* answers = new bool[4];
		std::string gameId = std::string(&strMess[17], &strMess[foundAnswers]);
		for(int i = 0; i < 4; i++){
			if(strMess.at(foundAnswers+9+2*i) == '1')
				answers[i] = true;
			else	
				answers[i] = false;
		}
		games[std::stoi(gameId)]->setAnswer(answers);		
	}
	else if(foundJoin == 0){
		size_t foundUser = strMess.find("\\user\\");
		std::string gameId = std::string(&strMess[14], &strMess[foundUser]);
		std::string gameUser = strMess.substr(foundUser+6);
		try{
			std::stoi(gameId);
		}
		catch(std::invalid_argument& e){
			sock->writeData("\\error\\id");
			return;
		}
		if(!isIdExists(std::stoi(gameId))){
			sock->writeData("\\error\\id");
			throw "Bad id";
		}
		else{
			Game* game = games[std::stoi(gameId)];
			if(!game->isValidUser(gameUser)){
				sock->writeData("\\error\\user");
				throw "Bad user";
			}
			else if(game->getGameStarted()){
				sock->writeData("\\error\\started");
				throw "Game already started";
			}
			else{
				std::string response = "\\ok\\game_name\\" + game->gameName + "\\quantity\\" + 
										std::to_string(game->getQNumber()) + "\\time\\" + 
										std::to_string(game->getTime());
				sock->game = game;
				sock->writeData(response);
				sock->writeData(game->getUsersMessage());
				game->addUser(gameUser, sock);
				game->broadcastToUsers("\\add_user\\"+gameUser, true);
			}
		} 
	}
	else if(foundStart == 0){
		size_t foundId = strMess.find("\\id\\");
		std::string gameId = strMess.substr(foundId+4);
		Game* game = games[std::stoi(gameId)];
		if(!game->startGame()){
			sock->writeData("\\error\\no_users");
		}
		else{
			sock->writeData("\\ok\\start_game\\");
			game->broadcastToUsers("\\next_question\\", false);
		}
	}
	else if(foundNext == 0){
		size_t foundId = strMess.find("\\id\\");
		std::string gameId = strMess.substr(foundId+4);
		Game* game = games[std::stoi(gameId)];
		game->nextQuestion();
		game->broadcastToUsers("\\next_question\\", false);
	}
	else if(foundStudentAnswer == 0){
		size_t foundId = strMess.find("\\id\\");
		size_t foundNumber = strMess.find("\\number\\");
		size_t foundAnswer = strMess.find("\\answer\\");
		std::string gameId = std::string(&strMess[foundId + 4], &strMess[foundNumber]);
		std::string number = std::string(&strMess[foundNumber + 8], &strMess[foundAnswer]);
		std::string answer = strMess.substr(foundAnswer + 8);
		Game* game = games[std::stoi(gameId)];
		game->checkAnswer(sock->sock, std::stoi(number) - 1, std::stoi(answer));
	}
	else if(foundEndGame == 0){
		size_t foundId = strMess.find("\\id\\");
		std::string gameId = strMess.substr(foundId + 4);
		Game* game = games[std::stoi(gameId)];
		game->broadcastToUsers("\\end_game\\", false);
		game->disconnectAllUsers();
		sock->socketClose();
		games.erase(sock->game->getGameId());
		delete sock->game;
	}
	else if(foundCountdown == 0){
		size_t foundId = strMess.find("\\id\\");
		std::string gameId = strMess.substr(foundId + 4);
		Game* game = games[std::stoi(gameId)];
		game->socket->writeData("\\question_end\\admin");
		game->broadcastToUsers("\\question_end\\user", false);
		game->broadcastToUsers(game->getUserPoints(), true);
	}
}

bool isIdExists(int id){
	if(games.empty())
		return false;
	std::map<int,Game*>::iterator it;
	it = games.find(id);
	if(it != games.end()){
		return true;
	}
	return false;
}