#include "LightSwitch.h"

int main(int argc, char** argv) {   
    LightSwitch ls;
     ls.SwitchOn();
     ls.SwitchOff();
    // The following line will result in compile-time errors due to invalid state transitions
    // ls.SwitchOff();
    return 0;
}