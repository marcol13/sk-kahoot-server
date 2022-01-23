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

struct connection_t{
	int socket;
	//Game;
	std::string name;
};

// server socket
int servFd;

// client sockets
std::unordered_set<int> clientFds;

// handles SIGINT
void ctrl_c(int);

// sends data to clientFds excluding fd
void sendToAllBut(int fd, char * buffer, int count);

// converts cstring to port
uint16_t readPort(char * txt);

// sets SO_REUSEADDR
void setReuseAddr(int sock);

void waitForHosts(int serverFd, epoll_event event);

void readData();

int generateGameId();

void handleMessage(char* mess, int sock);

int epoll_fd;


int main(int argc, char ** argv){
	epoll_fd = epoll_create1(0);

	if(epoll_fd == -1){
		fprintf(stderr, "Failed to create epoll descriptor\n");
	}

	// get and validate port number
	if(argc != 2) error(1, 0, "Need 1 arg (port)");
	auto port = readPort(argv[1]);
	
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

		connection_t* connection = new connection_t;
		connection->socket = clientFd;
		connection->name = "";
		event.data.ptr = connection;
		
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

void sendToAllBut(int fd, char * buffer, int count){
	int res;
	decltype(clientFds) bad;
	for(int clientFd : clientFds){
		if(clientFd == fd) continue;
		res = write(clientFd, buffer, count);
		if(res!=count)
			bad.insert(clientFd);
	}
	for(int clientFd : bad){
		printf("removing %d\n", clientFd);
		clientFds.erase(clientFd);
		close(clientFd);
	}
}

void waitForHosts(int serverFd, epoll_event event){
	while(true){
		// prepare placeholders for client address
		sockaddr_in clientAddr{0};
		socklen_t clientAddrSize = sizeof(clientAddr);

		auto clientFd = accept(servFd, (sockaddr*) &clientAddr, &clientAddrSize);
		if(clientFd == -1) error(1, errno, "accept failed");

		// fcntl(clientFd, F_SETFL, fcntl(clientFd, F_GETFL) | O_NONBLOCK);
		
		
		// tell who has connected
		printf("new connection from: %s:%hu (fd: %d)\n", inet_ntoa(clientAddr.sin_addr), ntohs(clientAddr.sin_port), clientFd);

		epoll_ctl(clientFd, EPOLL_CTL_ADD, serverFd, &event);
	}
}

void readData(){
	epoll_event event;
	// event.data.fd = servFd;
	char buffer[512];
	int read_size;
	connection_t* connection;
	
	while(true){
		int resultCount = epoll_wait(epoll_fd, &event, 1, -1);
		if(event.events & EPOLLIN){
			std::memset(buffer, 0, 512);
			connection = (connection_t*)event.data.ptr;
			read_size = read(connection->socket, buffer, sizeof(buffer));
			if(read_size < 1){
				fprintf(stderr, "%s disconnected", connection->name.c_str());
				epoll_ctl(epoll_fd, EPOLL_CTL_DEL, connection->socket, NULL);
				continue;
			}
			printf("%s\n", buffer);
			handleMessage(buffer, connection->socket);
		}
	}
}

int generateGameId(){
	std::random_device rd;
	std::mt19937 gen(rd());
	std::uniform_int_distribution<> distr(100000, 999999);
	return distr(gen);
}

void handleMessage(char* mess, int sock){
	printf("%s\n", mess);
	std::string strMess = std::string(mess);

	printf("%s\n", strMess.c_str());
	size_t foundName = strMess.find("\\create_game\\");
	if(foundName == 0){
		size_t foundQuantity = strMess.find("\\quantity\\");
		size_t foundTime = strMess.find("\\time\\");
		std::string gameName = std::string(&strMess[13], &strMess[foundQuantity]);
		std::string gameQuantity = std::string(&strMess[foundQuantity+10], &strMess[foundTime]);
		std::string gameTime = strMess.substr(foundTime+6);
		//TODO Check if it is unique
		printf("GameName: %s\n", gameName.c_str());
		printf("GameQuantity: %s\n", gameQuantity.c_str());
		printf("GameTime: %s\n", gameTime.c_str());
		int gameId = generateGameId();
		printf("%d\n", gameId);
		if(int i = write(sock, (char*)gameId, sizeof(gameId)) < 0){
			printf("%d/n", "coś nie działa");
		}
	}
	// printf("%d\n",found);
}