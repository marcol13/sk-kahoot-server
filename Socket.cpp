#include "Socket.h"
#include "Game.h"

void Socket::readData(){
    std::memset(buffer, 0, 512);
    int read_size = read(sock, buffer, sizeof(buffer));
    
    if(read_size < 1){
        fprintf(stderr, "%d disconnected\n", sock);
        throw "client disconnected";
    }
    nowReaded += std::string(buffer, read_size);
    while(true){
        if(messageSize < 0){
            if(nowReaded.size() > sizeof(int)){
                messageSize = ntohl(*(int*)nowReaded.c_str());
                printf("message: %d", messageSize);
                nowReaded = nowReaded.substr(sizeof(int));
            }
            else
                break;
        }   
        if(messageSize <= nowReaded.size()){
            message.push_back(nowReaded.substr(0,messageSize));
            nowReaded = nowReaded.substr(messageSize);
            messageSize = -1;
        }
        else{
            break;
        }
    }
}

void Socket::writeData(std::string message){
    int size = htonl(message.size());
    if(write(sock, (char*)&size, sizeof(int)) < 0){
        printf("%s\n", "coś nie działa");
    }
    if(write(sock, message.c_str(), message.size()) < 0){
        printf("%s\n", "coś nie działa");
    }
}

void Socket::socketClose(){
    close(sock);
    printf("close\n");
}
