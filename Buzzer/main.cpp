#include "Buzzer.h"

int main(){
 Buzzer b;

 b.TurnOn();
 b.TurnOff();

 //it won't compile, typestate error
 // b.TurnOn();
 // b.TurnOn();
 return 0;
}
