#include "TypestateLibrary.h"
#include "Drawer.h"
#include "RedLed.h"

using TypestateLibrary::TypestateClassConnector;
using TypestateLibrary::State;
using TypestateLibrary::Typestate_Template;

class PillBox {
public:
    PillBox() {
        DrawersBox = new std::vector<Drawer*>();
    }
    void Activate_pillBox() {
        std::cout << "PillBox is active now!" << std::endl;
    }
    void addDrawers(Drawer* d) {
        DrawersBox->push_back(d);
    }
    Drawer* Process_System_Time(int h, int m) {
        for (Drawer* d : *DrawersBox) {
            if (h == d->get_the_hour() && m == d->get_minutes())
            {
                return d;
            }
        }
        return nullptr;
    }
    void Deactivate_Pill_Box() {
        // Deactivate the pill box
    }
    void Switch_ON(Drawer* d, int timesCalled = 0) {
        if(d != nullptr && timesCalled < 5) {
            redled.setRedLed("ON");
            std::cout << "It's time to take " << d->get_pill_name()<< std::endl;

            // Recursive call with the Drawer object
            Switch_ON(d, timesCalled + 1);
        }
    }
    void Switch_OFF(Drawer* d) {
        redled.setRedLed("OFF");
        std::cout << "Drawer that has the pill" << d->get_pill_name() << "is now Closed!";
    }
    void Blink(Drawer* d) {
        redled.setRedLed("Blinking");
        std::cout << "Drawer that has the pill" << d->get_pill_name() << "is now Open!";
    }
private:
    std::vector<Drawer*>* DrawersBox;
    RedLed redled;
};

BETTER_ENUM(domain, int,
    Idle, Active, NonActive, Pill_Time_On, RedLedON, RedLedOFF, RedLedBlinking);

    using PillBox_typestate = Typestate_Template<
    State<domain::Idle,&PillBox::Activate_pillBox,domain::Active>,
    State<domain::Active,&PillBox::Process_System_Time,domain::Pill_Time_On>,
    State<domain::Pill_Time_On,&PillBox::Switch_ON,domain::RedLedON>,
    State<domain::RedLedON,&PillBox::Switch_ON,domain::RedLedON>,
    State<domain::RedLedON,&PillBox::Blink,domain::RedLedBlinking>,
    State<domain::RedLedBlinking,&PillBox::Switch_OFF,domain::RedLedOFF>,
    State<domain::Active,&PillBox::Deactivate_Pill_Box,domain::NonActive>
    >;

using Pillbox = TypestateClassConnector<PillBox, PillBox_typestate>;