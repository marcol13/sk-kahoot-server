#include "Game.h"

Game::Game(std::string name, int gameId, int qNumber, int time, int socket){
    this->gameName = name;
    this->gameId = gameId;
    this->qNumber = qNumber;
    this->time = time;
    this->isGameReady = false;
    this->answers = new bool*[qNumber];
    this->socket = socket;
    for(int i = 0; i < qNumber; i++){
        this->answers[i] = new bool[4];
    }
    this->users = {};
    this->nSetAnswers = 0;
}

int Game::getQNumber(){
    return this->qNumber;
}

void Game::setQNumber(int n){
    this->qNumber = n;
}

int Game::getGameId(){
    return this->gameId;
}

void Game::setGameId(int id){
    this->gameId = id;
}

bool* Game::getAnswer(int n){
    return this->answers[n];
}

void Game::setAnswer(bool arr[4]){
    this->answers[this->nSetAnswers] = arr;
    if(this->nSetAnswers < this->qNumber)
        this->nSetAnswers++;
}

bool Game::getGameReady(){
    return this->isGameReady;
}

void Game::setGameReady(){
    this->isGameReady = true;
}

bool Game::isValidUser(std::string name){
    if(this->users.size() == 0)
        return true;
    else{
        for(const User& el : this->users){
            if(name == el.name)
                return false;
        }
        return true;
    }
}

bool Game::addUser(std::string name){
    if(isValidUser(name)){
        this->users.push_back(*(new User(name)));
        return true;
    }
    return false;
}

int Game::userPosition(std::string name){
    int i = 0;
    for(const User& el : this->users){
            if(name == el.name)
                return i;
            i++;
        }
    return -1;
}

bool Game::deleteUser(std::string name){
    if(userPosition(name) != -1){
        this->users.erase(this->users.begin() + userPosition(name));
        return true;
    }
    return false;
}

void Game::checkAnswer(std::string user, int q, int a){
    if(this->answers[q][a] == true){
        for(User el : this->users){
                if(user == el.name){
                    el.incrementScore();
                    break;
                }
        }
    }
}


