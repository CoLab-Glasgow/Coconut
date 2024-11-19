#include "LightSwitch.h"

int main(int argc, char** argv) {   
    lightswitch ls;
    (ls->*&LightSwitch::turnOn)();
    (ls->*&LightSwitch::turnOff)();
    // The following lines will result in compile-time errors due to invalid state transitions
    // (ls->*&LightSwitch::turnOn)();
    // (ls->*&LightSwitch::turnOff)();
    return 0;
}