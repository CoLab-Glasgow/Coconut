#include <iostream>
#include "../PillBox.h"

int main() {
    Pillbox* pillbox;
    Drawer* drawer1 = new Drawer("Calcium", 4, 120);
    Drawer* drawer2 = new Drawer("Zinc", 5, 60);

    // Adding multiple drawers
    (pillbox->*&PillBox::addDrawers)(drawer1);
    (pillbox->*&PillBox::addDrawers)(drawer2);

    // Activate pillbox
    (pillbox->*&PillBox::Activate_pillBox)();

    // Process system time for a specific drawer
    Drawer* d = (pillbox->*&PillBox::Process_System_Time)(4,120);

    // Turning LED on
    (pillbox->*&PillBox::Switch_ON)(d, 0);

    // Executing multiple blink operations
    (pillbox->*&PillBox::Blink)(d);
    (pillbox->*&PillBox::Blink)(d);

    // Safely turning off the LED before ending
    (pillbox->*&PillBox::Switch_OFF)(d);

    return 0;
}
