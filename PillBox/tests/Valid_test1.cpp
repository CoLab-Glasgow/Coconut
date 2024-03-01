#include <iostream>
#include "../PillBox.h"

int main() {
    Pillbox pillbox ;
    Drawer* drawer1 = new Drawer("Panadol", 3, 50);
    Drawer* drawer2 = new Drawer("Piriton Antihistamine", 8, 40);

    (pillbox->*&PillBox::addDrawers)(drawer1);
    (pillbox->*&PillBox::addDrawers)(drawer2);

    (pillbox->*&PillBox::Activate_pillBox)();
    Drawer* d = (pillbox->*&PillBox::Process_System_Time)(3,50);

    
    return 0;
}
