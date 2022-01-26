#include <string>

class Socket;

class User{
    private:
        int score;

    public:
        User(std::string, Socket*);

        std::string name;
        Socket* socket;
        
        int getScore();
        void setScore(int);
        void incrementScore();

};