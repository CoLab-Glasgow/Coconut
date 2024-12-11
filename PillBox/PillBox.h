#include <iostream>
#include <vector>
#include "Drawer.h"  
#include "RedLed.h"
#include "memory.h"
#include "../include/Typestate_Library/TypestateLibrary.h"

using TypestateLibrary::Template::TypestateClassConnector;
using TypestateLibrary::Template::State;
using TypestateLibrary::Template::Typestate_Template;


class PillBox {
public:
    PillBox() {
        // No need to initialize the vector explicitly, it's handled automatically
    }

    void Activate_pillBox() {
        std::cout << "PillBox is active now!\n";
    }

    // Add drawers using raw pointers
    void addDrawers(Drawer* d) {
        DrawersBox.push_back(d);  // Store raw pointers in the vector
    }

    Drawer* Process_System_Time(int h, int m) {
        for (Drawer* d : DrawersBox) {  // Iterate over raw pointers
            if (h == d->get_the_hour() && m == d->get_minutes()) {
                return d;  // Return the raw pointer
            }
        }
        return nullptr;
    }

    void Deactivate_Pill_Box() {
        std::cout << "PillBox is Deactivated now!\n";
    }

    void Switch_ON(Drawer* d) {
        redled.setRedLed("ON");
        std::cout << "It's time to take " << d->get_pill_name() << "!\n";
    }

    void Switch_OFF(Drawer* d) {
        redled.setRedLed("OFF");
        d->SetDrawerState("CLOSED");
        std::cout << "Drawer with the pill " << d->get_pill_name() << " is closed\n";
    }

    void Blink(Drawer* d) {
        redled.setRedLed("Blinking");
        std::cout << "Please close the drawer\n";
    }

    // Destructor to clean up manually allocated memory
    ~PillBox() {
        for (Drawer* d : DrawersBox) {
            delete d;  // Manually free each Drawer
        }
    }

private:
    std::vector<Drawer*> DrawersBox;  // Store raw pointers to Drawer objects
    RedLed redled;
};


// Define the normal enum with integer values
enum class domain {
    Idle = 0,
    Active = 1,
    NonActive = 2,
    Pill_Time_On = 3,
    RedLedON = 4,
    RedLedOFF = 5,
    RedLedBlinking = 6
};

// Define the state transitions using the updated enum class 'domain'
using PillBox_typestate = Typestate_Template<
    State<domain::Idle, &PillBox::Activate_pillBox, domain::Active>,
    State<domain::Active, &PillBox::Process_System_Time, domain::Pill_Time_On>,
    State<domain::Pill_Time_On, &PillBox::Switch_ON, domain::RedLedON>,
    State<domain::RedLedON, &PillBox::Switch_ON, domain::RedLedON>,
    State<domain::RedLedON, &PillBox::Blink, domain::RedLedBlinking>,
    State<domain::RedLedON, &PillBox::Switch_OFF, domain::RedLedOFF>,
    State<domain::RedLedBlinking, &PillBox::Switch_OFF, domain::RedLedOFF>,
    State<domain::RedLedOFF, &PillBox::Switch_ON, domain::RedLedON>,
    State<domain::RedLedOFF, &PillBox::Deactivate_Pill_Box, domain::NonActive>,
    State<domain::Active, &PillBox::Deactivate_Pill_Box, domain::NonActive>
>;


using TypestateClassConnectorPillbox = TypestateClassConnector<PillBox, PillBox_typestate>;

TypestateClassConnectorPillbox FS;
void ttt(){
    PillBox_typestate t;
    t.display();

    FS.display();
}