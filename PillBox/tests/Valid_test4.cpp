#include <iostream>
#include "../PillBox.h"

int main() {
    Pillbox* pillbox;
    Drawer* drawer1 = new Drawer("Calcium", 4, 120);
    Drawer* drawer2 = new Drawer("Zinc", 5, 60);

    // Adding multiple drawers
    (pillbox->*&PillBox::addDrawers)(std::move(drawer1));
    (pillbox->*&PillBox::addDrawers)(std::move(drawer2));

    // Activate pillbox
    (pillbox->*&PillBox::Activate_pillBox)();

    // Process system time for a specific drawer
    Drawer* d = (pillbox->*&PillBox::Process_System_Time)(4,120);

    // Turning LED on
    (pillbox->*&PillBox::Switch_ON)(std::move(d), 0);

    // Executing multiple blink operations
    (pillbox->*&PillBox::Blink)(std::move(d));
    (pillbox->*&PillBox::Blink)(std::move(d));

    // Safely turning off the LED before ending
    (pillbox->*&PillBox::Switch_OFF)(std::move(d));

    
    return 0;
}
