#include "../LightSwitch.h"

int main(int argc, char** argv) {   
    lightswitch ls;
    (ls->*&LightSwitch::turnOn)();
    (ls->*&LightSwitch::turnOn)();
    return 0;
}