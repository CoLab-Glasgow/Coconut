#include<iostream>
#include <thread>
#include "PillBox.h"

int main() {
    //PillBox_typestate pp; 
    //pp.Visualize_TypestateTemplate();
    Drawer* drawer1 = new Drawer("Panadol", 3, 50);
    Drawer* drawer2 = new Drawer("Piriton Antihistamine", 8, 40);
    Pillbox pillbox1;
    Pillbox& ptr_pillbox1 = pillbox1;
    (pillbox1->*&PillBox::addDrawers)(drawer1);
    (pillbox1->*&PillBox::addDrawers)(drawer1);
    (pillbox1->*&PillBox::Activate_pillBox)();
    Drawer* d = (pillbox1->*&PillBox::Process_System_Time)(3,50);
    int i=0;
    while (i<5){
    (ptr_pillbox1->*&PillBox::Switch_ON)(d);
     i++;
    }
    (pillbox1->*&PillBox::Blink)(d);
    (ptr_pillbox1->*&PillBox::Switch_OFF)(d);
    return 0;
}
