#include <iostream>
#include "../PillBox.h"

int main() {
    Pillbox* pillbox;
    Drawer* drawer = new Drawer("Paracetamol", 4, 250);

    (pillbox->*&PillBox::addDrawers)(std::move(drawer));

    (pillbox->*&PillBox::Activate_pillBox)();
    Drawer* d = (pillbox->*&PillBox::Process_System_Time)(4,250);
    // Missing switch on step
    (pillbox->*&PillBox::Blink)(std::move(d)); // This operation is invalid without switching on the LED first.
    
    return 0;
}
