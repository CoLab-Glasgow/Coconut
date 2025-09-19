#include "Operator.h"

int getSensorReading (){
    return 1;
}
int main ()
{
 Robot robot1 , robot2 ; // both at state 0
robot1.StartNavigation ("shelf03901");// robot 1 here is at state 1
Robot& r2 = robot2;
Robot* ptr = new Robot();
 int currentZone = getSensorReading(); // runtime sensor check
 if ( currentZone ==0) {
 ptr = &robot1 ; }
 else {
 ptr = &robot2; }

 ptr->PickUpObject("Box1837"); // Warning 

 return 0;
}
