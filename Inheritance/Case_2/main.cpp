 #include "Vehicle.h"
 #include "Car.h"
 int main() {
	 vehicle V1; 
	 Car C;
	 (V1->*&Vehicle::Start)();
	 (V1->*&Vehicle::Set_Speed)(70);
	 (V1->*&Vehicle::Set_Speed)(50);
     (V1->*&Vehicle::Stop)();
       
     (C->*&Vehicle::Start)();
     (C->*&Vehicle::Set_Speed)(70);
	 (C->*&Car::Play_Music)();
	 (C->*&Vehicle::Set_Speed)(30);
	 (C->*&Car::Adjust_Seats)();
	 (C->*&Vehicle::Stop)();
	 return 0;
 }
