#include <iostream>
#include "../PillBox.h"

int main() {
    Pillbox* pillbox; 
    Drawer* morningDrawer = new Drawer("Vitamin D3", 1, 100);
    Drawer* eveningDrawer = new Drawer("Magnesium", 2, 200);

    // Adding drawers for different times
    (pillbox->*&PillBox::addDrawers)(morningDrawer);
    (pillbox->*&PillBox::addDrawers)(eveningDrawer);

    // Morning cycle
    (pillbox->*&PillBox::Activate_pillBox)();
    Drawer* d1 = (pillbox->*&PillBox::Process_System_Time)(1,100);
    (pillbox->*&PillBox::Switch_ON)(d1, 0);
    (pillbox->*&PillBox::Blink)(d1);
    (pillbox->*&PillBox::Switch_OFF)(d1);
    (pillbox->*&PillBox::Deactivate_Pill_Box)();

    // Evening cycle
    (pillbox->*&PillBox::Activate_pillBox)();
    Drawer* d2 = (pillbox->*&PillBox::Process_System_Time)(2,200);
    (pillbox->*&PillBox::Switch_ON)(d2, 0);
    (pillbox->*&PillBox::Blink)(d2);
    (pillbox->*&PillBox::Switch_OFF)(d2);
    (pillbox->*&PillBox::Deactivate_Pill_Box)();

    
    return 0;
}
