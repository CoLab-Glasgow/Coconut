

#include<iostream>
#include "PillBox.h"
int main() {
    Drawer* drawer1 = new Drawer("Panadol", 3, 50);
    Drawer* drawer2 = new Drawer("Piriton Antihistamine", 8, 40);
    Pillbox pillbox1;
    Pillbox& ptr_pillbox1 = pillbox1;
    (pillbox1->*&PillBox::addDrawers)(std::move(drawer1));
    (pillbox1->*&PillBox::addDrawers)(std::move(drawer2));
    (pillbox1->*&PillBox::Activate_pillBox)();
     Drawer* d = (ptr_pillbox1->*&PillBox::Process_System_Time)(3,50);
    (pillbox1->*&PillBox::Switch_ON)(std::move(d),0);
    (pillbox1->*&PillBox::Blink)(std::move(d));
    (ptr_pillbox1->*&PillBox::Switch_OFF)(std::move(d));
    return 0;
}
