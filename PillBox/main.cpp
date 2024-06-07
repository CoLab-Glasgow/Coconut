#include<iostream>
#include <thread>
#include "PillBox.h"

static void OperateRedLed(Pillbox& p, Drawer* d){
   for(int i=0;i<5;i++){
     (p->*&PillBox::Switch_ON)(d);
     }
    (p->*&PillBox::Blink)(d);
    (p->*&PillBox::Switch_OFF)(d);
}

int main() {
    //PillBox_typestate pp; 
    //pp.Visualize_TypestateTemplate();

    Drawer* drawer1 = new Drawer("Panadol", 3, 50);
    Drawer* drawer2 = new Drawer("Piriton Antihistamine", 8, 40);
    constexpr bool EnableDrawersOperations = true; 
    Pillbox pillbox1;
    Pillbox& ptr_pillbox1 = pillbox1;
    pillbox1.addDrawers(drawer1);
    pillbox1.addDrawers(drawer2);
    (pillbox1->*&PillBox::Activate_pillBox)();
    if constexpr (EnableDrawersOperations) {
    Drawer* d = (pillbox1->*&PillBox::Process_System_Time)(3,50);
     OperateRedLed(ptr_pillbox1, d);
    }
    else {
    (pillbox1->*&PillBox::Deactivate_Pill_Box)();
     }

return 0;
}
