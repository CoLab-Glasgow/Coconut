
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


