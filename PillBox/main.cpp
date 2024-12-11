#include <iostream>
#include "PillBox.h"

int main() {

    Drawer* drawer1 = new Drawer("Panadol", 3, 50);  
    Drawer* drawer2 = new Drawer("Piriton Antihistamine", 8, 40);  

    PillBox pillbox1;

    
    pillbox1.addDrawers(drawer1);
    pillbox1.addDrawers(drawer2);


    pillbox1.Activate_pillBox();
    int hours = 3;
    int minutes = 50;
    Drawer* d = pillbox1.Process_System_Time(hours, minutes);
    
    pillbox1.Switch_ON(d);  
    pillbox1.Blink(d);      
    pillbox1.Switch_OFF(d); 

       
   pillbox1.Deactivate_Pill_Box(); 
    
    return 0;
}
