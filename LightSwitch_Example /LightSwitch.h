#include <cstdlib>
#include <iostream>
#include "TypestateLibrary.h" 

using TypestateLibrary::TypestateClassConnector;
using TypestateLibrary::State;
using TypestateLibrary::Typestate_Template;

class LightSwitch {
public:
    void turnOn() {
        std::cout << "Switching On\n";
    }

    void turnOff() {
        std::cout << "Switching Off\n";
    }
};

BETTER_ENUM(LightSwitchState, int, 
    OFF,
    ON
)

using LightSwitchProtocol = Typestate_Template<
    State<+LightSwitchState::OFF, &LightSwitch::turnOn, +LightSwitchState::ON>,
    State<+LightSwitchState::ON, &LightSwitch::turnOff, +LightSwitchState::OFF>
>;

using lightswitch = TypestateClassConnector<LightSwitch, LightSwitchProtocol>;

