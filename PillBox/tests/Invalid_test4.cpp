#include <iostream>
#include "../PillBox.h"

int main() {
    Pillbox* pillbox; 
    Drawer* drawer = new Drawer("Melatonin", 1, 300);

    (pillbox->*&PillBox::addDrawers)(std::move(drawer));

    (pillbox->*&PillBox::Activate_pillBox)();
    Drawer* d = (pillbox->*&PillBox::Process_System_Time)(1,300);
    (pillbox->*&PillBox::Switch_ON)(std::move(d),0);
    // Deactivating while LED is on (missing switch off step)
    (pillbox->*&PillBox::Deactivate_Pill_Box)(); // This operation might be considered invalid as per state machine.
    return 0;
}
