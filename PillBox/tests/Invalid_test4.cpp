#include <iostream>
#include "../PillBox.h"

int main() {
    Pillbox* pillbox; 
    Drawer* drawer = new Drawer("Melatonin", 1, 300);

    (pillbox->*&PillBox::addDrawers)(drawer);

    (pillbox->*&PillBox::Activate_pillBox)();
    Drawer* d = (pillbox->*&PillBox::Process_System_Time)(1,300);
    (pillbox->*&PillBox::Switch_ON)(d,0);
    // Deactivating while LED is on (missing switch off step)
    (pillbox->*&PillBox::Deactivate_Pill_Box)(); // This operation might be considered invalid as per state machine logic.

    delete pillbox; // Assuming PillBox class handles deletion of drawers.
    return 0;
}
