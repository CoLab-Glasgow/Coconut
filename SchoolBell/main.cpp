#include "SchoolBell.h"

int main() {
 SchoolBell bell;      
   
    bell.TurnOn();

    bell.notify = true;
    bell.error  = false;

    // Must call checkStatus() while in ON
    bool shouldBeep = bell.checkStatus();    // ON → checkStatus() → stays in ON

    if (shouldBeep) {
      
        bell.Beep();   
        bell.StopBeep();
    }
    else {
        // ON → TriggerFault() → FAULT
        bell.TriggerFault();
        // FAULT → Reset() → ON
        bell.Reset();
    }

   
    // Finally: ON → TurnOff() → OFF
    bell.TurnOff();

    return 0;
}
