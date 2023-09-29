#include<iostream>

class Drawer {
public:

    Drawer(std::string pillName, int hour, int minute)
        : pillName_(pillName), hour_(hour), minute_(minute) {}

    std::string get_pill_name()
    {
        return this->pillName_;

    }
    int get_the_hour()
    {

        return this->hour_;
    }
    int get_minutes() {
        return this->minute_;
    }
private:
    std::string pillName_;
    int hour_;
    int minute_;


};
