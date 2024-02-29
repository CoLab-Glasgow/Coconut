#include "LightSwitch.h"

int main(int argc, char** argv) {
    Lightswitch ls;
    (ls->*&LightSwitch::turnOn)(); 
    (ls->*&LightSwitch::turnOff)();

    return 0;
}
