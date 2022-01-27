#include "User.h"


#include <string>
#include <vector>
#include <unistd.h>

class Socket;

class Game{
    private:
        int qNumber;
        int gameId;
        int time;
        bool** answers;
        bool isGameReady;
        bool isGameStarted;
        int nSetAnswers;

    public:
        Game(std::string, int, int, int, Socket*);
        // ~Game();

        std::string gameName;
        std::vector<User> users;
        Socket* socket;

        int getQNumber();
        void setQNumber(int);

        int getTime();
        
        int getGameId();
        void setGameId(int);

        bool* getAnswer(int);
        void setAnswer(bool[4]);

        bool getGameReady();
        void setGameReady();

        bool isValidUser(std::string);
        bool addUser(std::string, Socket*);
        int userPosition(std::string);
        bool deleteUser(std::string);

        //getResults
        //sendResults()

        bool isHost(int);
        void onHostDisconnected();
        void onPlayerDisconnected(int);


        std::string getUsersMessage();
        void broadcastToUsers(std::string);

        

        //a - [0,3];
        void checkAnswer(std::string, int q, int a);
};