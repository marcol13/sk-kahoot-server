#include "User.h"
#include <string>

User::User(std::string name){
    setScore(0);
    this->name = name;
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