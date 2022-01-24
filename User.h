#include <string>


class User{
    private:
        int score;

    public:
        User(std::string);

        std::string name;
        
        int getScore();
        void setScore(int);
        void incrementScore();

};