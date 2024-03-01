#include <iostream>
#include "../PillBox.h"

int main() {
    Pillbox pillbox;
    Drawer* drawer = new Drawer("Aspirin", 2, 100);

    (pillbox->*&PillBox::addDrawers)(std::move(drawer));

    // Missing activation step
    Drawer* d = (pillbox->*&PillBox::Process_System_Time)(2,10); // This should fail or be considered invalid.

   
    return 0;
}
