

#include<iostream>
#include "PillBox.h"

int main() {

    PillBox pill;
    

    Drawer drawer1("Panadol", 3, 50);
    Drawer drawer2("Piriton Antihistamine", 8, 40);
    int h = 10;
    int min = 30;
    Pillbox pillbox1;
    Pillbox& ptr_pillbox1 = pillbox1;
   
    //PillBox().Process_System_Time();
    //pillbox1.Activate_pillBox();
    (pillbox1->* & PillBox::Activate_pillBox)();
    Drawer* d = (ptr_pillbox1->* & PillBox::Process_System_Time)();


  
    return 0;

}
