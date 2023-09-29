
#include <iostream>
#include <string>
#include <chrono>
#include <thread>
#include <vector>
#include "TypestateLibrary.h"

using TypestateLibrary::TypestateClassConnector;
using TypestateLibrary::State;
using TypestateLibrary::Typestate_Template;




class PillBox {
public:
    PillBox() {
        DrawersBox = new std::vector<Drawer*>();
    }

    ~PillBox() {
        delete DrawersBox;
    }

    void Activate_pillBox() {
        std::cout << "PillBox is active now!" << std::endl;
    }

    void addDrawers(Drawer* d) {
        DrawersBox->push_back(d);
    }

    Drawer* Process_System_Time() {
        int h = 3;
        int m = 50;
        for (Drawer* d : *DrawersBox) {
            if (h == d->get_the_hour() && m == d->get_minutes()) {
                return d;
            }
        }
        return nullptr; // Return nullptr if no matching drawer is found
    }
    void Deactivate_Pill_Box() {
        // Deactivate the pill box
    }
private:
    std::vector<Drawer*>* DrawersBox;
};



class RedLed {

public:

    void Switch_ON(Drawer* d) {
        std::cout << "It's time to take the" <<d->get_pill_name();
    }
    void Switch_OFF(Drawer* d) {
        std::cout << "Drawer that has the pill " << d->get_pill_name() << " is now Closed!"
            << std::endl;
    }
    void Blink(Drawer* d) {
        std::cout << "Drawer that has the pill " << d->get_pill_name() << " is now Open!"
            << std::endl;
    }



};



BETTER_ENUM(domain, int,
    Idle, Active, NonActive, Pill_Time_On, RedLedON, RedLedOFF, RedLedBlinkinning)


    using PillBox_typestate = Typestate_Template<
    State<domain::Idle, &PillBox::Activate_pillBox, domain::Active>,
    State<domain::Active, &PillBox::Process_System_Time, domain::Pill_Time_On>,
    State<domain::Pill_Time_On, &RedLed::Switch_ON, domain::RedLedON>,
    State<domain::RedLedON, &RedLed::Switch_ON, domain::RedLedON>,
    State<domain::RedLedON, &RedLed::Blink, domain::RedLedBlinkinning>,
    State<domain::RedLedBlinkinning, &RedLed::Switch_OFF, domain::RedLedOFF>,
    State<domain::Active, &PillBox::Deactivate_Pill_Box, domain::NonActive>
    >;

using Pillbox = TypestateClassConnector<PillBox, PillBox_typestate>;
using Redled = TypestateClassConnector<RedLed, PillBox_typestate>;

int main() {


Drawer drawer1("Panadol", 3, 50);
Drawer drawer2("Piriton Antihistamine", 8, 40);


int h = 10;
int min = 30;


Pillbox pillbox1 , pillbox2;
Redled redled;

(pillbox1->* &PillBox::addDrawers)(&drawer1);
(pillbox2->* &PillBox::addDrawers)(&drawer2);
Drawer* d = (pillbox1->* & PillBox::Process_System_Time)();
(pillbox1->* & PillBox::Activate_pillBox)();
   
(redled->*&RedLed::Switch_ON)(std::move(d));
(redled->*&RedLed::Blink)(std::move(d));
(redled->*&RedLed::Switch_OFF)(std::move(d));






    return 0;

}
