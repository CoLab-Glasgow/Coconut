#include "LightSwitch.h"

int main(int argc, char** argv) {   
    lightswitch ls;
    (ls->*&LightSwitch::turnOff)();
    (ls->*&LightSwitch::turnOff)();
    return 0;
}