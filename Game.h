#include "User.h"

#include <string>
#include <vector>

class Game{
    private:
        int qNumber;
        int gameId;
        int time;
        bool** answers;
        bool isGameReady;
        int nSetAnswers;

    public:
        Game(std::string, int, int, int, int);
        // ~Game();

        std::string gameName;
        std::vector<User> users;
        int socket;

        int getQNumber();
        void setQNumber(int);
        
        int getGameId();
        void setGameId(int);

        bool* getAnswer(int);
        void setAnswer(bool[4]);

        bool getGameReady();
        void setGameReady();

        bool isValidUser(std::string);
        bool addUser(std::string);
        int userPosition(std::string);
        bool deleteUser(std::string);

        //a - [0,3];
        void checkAnswer(std::string, int q, int a);
};