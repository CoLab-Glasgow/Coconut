#include<iostream>

class Drawer {
public:
    Drawer(const char* pillName, int hour, int minute)
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
    const char* GetDrawerState(){
      return DrawerState;
    }
    void SetDrawerState(const char* state){
        this->DrawerState=state;
    }
    
private:
    const char* pillName_;
    int hour_;
    int minute_;
    const char* DrawerState = "CLOSED";
};