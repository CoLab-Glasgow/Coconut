 #include "Vehicle.h"
 #include "Car.h"
 int main() { 
	 vehicle V1; 
	 Car C;
	 // will compile it follows the typestates
	 (V1->*&Vehicle::Start)();
	 (V1->*&Vehicle::Set_Speed)(70);
	 (V1->*&Vehicle::Set_Speed)(50);
         (V1->*&Vehicle::Stop)();
       
         (C->*&Vehicle::Start)();
         (C->*&Vehicle::Set_Speed)(70);
	 (C->*&Vehicle::Set_Speed)(30);
	 (C->*&Vehicle::Stop)();
	 return 0;
 }
