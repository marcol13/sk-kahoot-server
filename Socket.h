#include <string>
#include <unistd.h>
#include <cstring>
#include <list>
#include <arpa/inet.h>

class Socket{
    public:
        int sock;
        char buffer[512];
        std::string nowReaded;
        int messageSize = -1;
        std::list<std::string> message;


        void readData();
        void writeData(std::string);
        void socketClose();
};