#include "../LightSwitch.h"

int main(int argc, char** argv) {
    Lightswitch ls;
    (ls->*&LightSwitch::turnOff)(); 
    (ls->*&LightSwitch::turnOn)();

    return 0;
}
