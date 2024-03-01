#include <iostream>
#include "../PillBox.h"

int main() {
    Pillbox pillbox;
    Drawer* drawer = new Drawer("Vitamin C", 10, 500);
    (pillbox->*&PillBox::addDrawers)(std::move(drawer));
    (pillbox->*&PillBox::Activate_pillBox)();
    // Missing process time step
    (pillbox->*&PillBox::Switch_ON)(std::move(drawer),0); // This operation is invalid without processing time first.
    return 0;
}
