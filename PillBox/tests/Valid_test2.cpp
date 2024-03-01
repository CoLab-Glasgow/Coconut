#include <iostream>
#include "../PillBox.h"

int main() {
    Pillbox pillbox; 
    Drawer* drawer = new Drawer("Ibuprofen", 5, 200);

    (pillbox->*&PillBox::addDrawers)(std::move(drawer));

    (pillbox->*&PillBox::Activate_pillBox)();
     Drawer* d = (pillbox->*&PillBox::Process_System_Time)(5,200);
    (pillbox->*&PillBox::Switch_ON)(d,0);
    (pillbox->*&PillBox::Blink)(d);
    (pillbox->*&PillBox::Switch_OFF)(d);

    return 0;
}
