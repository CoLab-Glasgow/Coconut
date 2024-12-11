#include <cstdlib>
#include <iostream>
//#include "TypestateLibrary.h" 

#include "../include/Typestate_Library/TypestateLibrary.h"

using TypestateLibrary::Template::TypestateClassConnector;
using TypestateLibrary::Template::State;
using TypestateLibrary::Template::Typestate_Template;



class LightSwitch {
public:
 void SwitchOn() {
    std::cout<<"Switching On\n";
    }
 void SwitchOff() {
    std::cout<<"Switching Off\n";
    }
};

enum class LightSwitchState{
    OFF=0,
    ON=1,
};

using LightSwitchProtocol = Typestate_Template<

    State<LightSwitchState::OFF, &LightSwitch::SwitchOn, LightSwitchState::ON>,

    State<LightSwitchState::ON,  &LightSwitch::SwitchOff, LightSwitchState::OFF>
>;

using lightswitchFlag = TypestateClassConnector<LightSwitch, LightSwitchProtocol>;

lightswitchFlag lSP;
LightSwitchProtocol LSP;
void init(){
j.display();
lp.display();
}
