 
 #include "Vehicle.h"
 #include "Car.h"
 int main() {
	 
	 vehicle V1; 
	 Car C;
	 // will compile it follows the typestates
	 (V1->* & Vehicle::Set_Speed)(70);
	 (V1->* &Vehicle::Set_Speed)(50);
         (V1->* &Vehicle::Stop)();
        // will not compile
       //(V1->* &Vehicle::SetSpeed)(70);
      // the subclass also adhere to the typestates
        (C->*&Vehicle::Set_Speed)(70);
       (C->*&Car::Adjust_Seats)();
       // this is an example of subclass that does not follow the typestate
	 // it won't compile
	 //(C->*&Car::StopTheVehicle)();
	 //(C->*&Car::SetSpeed)(70);
	


	 return 0;
 }
