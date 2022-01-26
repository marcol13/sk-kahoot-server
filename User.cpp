#include "User.h"
#include "Socket.h"

User::User(std::string name, Socket* socket){
    setScore(0);
    this->name = name;
    this->socket = socket;
}

int User::getScore(){
    return this->score;
}

void User::setScore(int n){
    this->score = n;
}

void User::incrementScore(){
    this->score++;
}