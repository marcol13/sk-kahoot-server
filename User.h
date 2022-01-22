#include <string>


class User{
    private:
        int score;

    public:
        User(std::string);
        ~User();

        std::string name;
        
        int getScore();
        void setScore(int);
        void incrementScore();

};