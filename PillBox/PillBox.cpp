#include "PillBox.h"
#include <iostream>
#include <string>
#include <chrono>
#include <thread>
#include <vector>
#include "C:\Users\Arwa Alsubhi\source\repos\Cocount_Typestate_based_tool\Cocount_Typestate_based_tool\TypestateLibrary.h"

using TypestateLibrary::TypestateClassConnector;
using TypestateLibrary::State;
using TypestateLibrary::Typestate_Template;



class Drawer {
public :

    Drawer(std::string pillName, int hour, int minute)
        : pillName_(pillName), hour_(hour), minute_(minute) {}

    std::string get_pill_name() {
        return this->pillName_; 
    }
    int get_the_hour() {
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



class PillBox {

public:
    void activate_pillBox() {
        std::cout << "PillBox is active now! " << std::endl;

    }
    void addDrawers(Drawer* d) {

       DrawersBox.push_back(*d);
    }

    Drawer Process_System_Time(int h, int m) {
        for (auto& d : DrawersBox) {
            if (h == d.get_the_hour() && m == d.get_minutes()) {
                return d;
            }
        }

    }
 
    void Deactivate_Pill_Box() {

    }

private:

    std::vector<Drawer> DrawersBox;
};


class RedLed {

public:
    
    void Swich_ON(Drawer* d) {
        std::cout << "It's time to take the" << d->get_pill_name() << "! " << "\n";
    }
    void Swich_OFF(Drawer* d) {
        std::cout << "Drawer that has the pill " << d->get_pill_name() << " is now Closed!"
            << std::endl;
    }
    void Blink(Drawer* d) {
        std::cout << "Drawer that has the pill " << d->get_pill_name() << " is now Open!"
            << std::endl;
    }



};



BETTER_ENUM(domain , int, 
    Idle, Active,NonActive, Pill_Time_On,RedLedON, RedLedOFF, RedLedBlinkinning)
    

 using PillBox_typestate = Typestate_Template<
    State<domain::Idle, &PillBox::activate_pillBox, domain::Active>,
    State<domain::Active, &PillBox::Process_System_Time, domain::Pill_Time_On>,
    State<domain::Pill_Time_On, &RedLed::Swich_ON, domain::RedLedON>,
    State<domain::RedLedON, &RedLed::Swich_ON, domain::RedLedON>,
    State<domain::RedLedON, &RedLed::Blink, domain::RedLedBlinkinning>,
    State<domain::RedLedBlinkinning, &RedLed::Swich_OFF, domain::RedLedOFF>,
    State<domain::Active, &PillBox::Deactivate_Pill_Box, domain::NonActive>
 > ;

using PillBox_ = TypestateClassConnector<PillBox, PillBox_typestate>;
using RedLed_  = TypestateClassConnector<RedLed, PillBox_typestate>;

int main() {

   
    Drawer drawer1("Panadol", 3, 50);
    Drawer drawer2("Piriton Antihistamine", 8, 40);
   

   auto h = 10;
   auto min = 30; 


   PillBox_ pillbox;
   RedLed_ redled;
  
   (pillbox->* & PillBox::addDrawers)(&drawer1);
   (pillbox->* & PillBox::addDrawers)(&drawer2);

  (pillbox->* & PillBox::activate_pillBox)();
   Drawer d = (pillbox->* &PillBox::Process_System_Time)(std::move(h), std::move(min));
  (redled->* &RedLed::Swich_ON)(&d);
  (redled->* &RedLed::Blink)(&d);
  (redled->* &RedLed::Swich_OFF)(&d);

   
  
   


    return 0;

}

