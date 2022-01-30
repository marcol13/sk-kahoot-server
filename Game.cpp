#include "Game.h"
#include "Socket.h"

Game::Game(std::string name, int gameId, int qNumber, int time, Socket* socket){
    this->gameName = name;
    this->gameId = gameId;
    this->qNumber = qNumber;
    this->time = time;
    this->isGameReady = false;
    this->isGameStarted = false;
    this->answers = new bool*[qNumber];
    this->socket = socket;
    this->nQuizQuestion = 0;
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

int Game::getTime(){
    return this->time;
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
    if(this->nSetAnswers < this->qNumber){
        this->answers[this->nSetAnswers] = arr;
        this->nSetAnswers++;
    }   
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
        for(User* el : this->users){
            if(name == el->name)
                return false;
        }
        return true;
    }
}

bool Game::addUser(std::string name, Socket* sock){
    if(isValidUser(name)){
        this->users.push_back(new User(name, sock));
        return true;
    }
    return false;
}

int Game::userPosition(std::string name){
    int i = 0;
    for(User* el : this->users){
            if(name == el->name)
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

std::string Game::getUsersMessage(){
    std::string mess = "\\users";
    if(this->users.size() == 0)
        mess += "\\";
    else{
        for(auto &user : this->users){
            mess += "\\" + user->name;
        }
    }
    return mess;
}

void Game::broadcastToUsers(std::string mess, bool withAdmin){
    if(withAdmin)
        socket->writeData(mess);
    for(auto &user : this->users){
        if(!user->isClosed)
            user->socket->writeData(mess);
    }
}

void Game::checkAnswer(int sock, int q, int a){
    if(q == this->nQuizQuestion){
        if(this->answers[q][a] == true){
            for(User* el : this->users){
                    if(sock == el->socket->sock){
                        el->incrementScore();
                        break;
                    }
            }
        }
    }
}

bool Game::isHost(int sock){
    if(sock == socket->sock)
        return true;
    else 
        return false;
}

void Game::onHostDisconnected(){
    broadcastToUsers("\\unreachable_host\\", false);
    disconnectAllUsers();
}

void Game::onPlayerDisconnected(int sock){
    std::string user;
    bool before = false;
    for(User* el : this->users){
        if(sock == el->socket->sock){ 
            if(!isGameStarted){
                user = el->name;
                before = true;
            }
            else{
                el->isClosed = true;
            }
            break;
        }
    }
    if(before){
    deleteUser(user);
    broadcastToUsers("\\delete_user\\"+user, true);
    }
}

bool Game::startGame(){
    if(users.empty())
        return false;
    else
    {
        setGameReady();
        this->isGameStarted = true;
        return true;
    }
}

bool Game::getGameStarted(){
    return this->isGameStarted;
}

void Game::nextQuestion(){
    this->nQuizQuestion++;
}

std::string Game::getUserPoints(){
    std::string command = "\\game_results";
    for(User* el : this->users){
        command += "\\name\\" + el->name;
        command += "\\score\\" + std::to_string(el->getScore());
    }
    return command;
}

void Game::disconnectAllUsers(){
    for(User* el : this->users){
        el->socket->socketClose();
    }
}