

#ifndef Vehicle_HEADER
#define Vehicle_HEADER




#include <Typestate_Library\TypestateLibrary.h>


using TypestateLibrary::TypestateClassConnector;
using TypestateLibrary::State;
using TypestateLibrary::Typestate_Template;

// case 1 A class without typestate inherits from a class with typestate. 

class Vehicle
{
	long speed;

public:
	void SetSpeed(long s) {
		std::cout << "Vehicle speed is: " << s << std::endl;;
		this->speed = s;
	}

	void StopTheVehicle() {
		std::cout << "Vehicle Stopped !";

	}


};



enum VehicleStates {
	IDEL,
	START,
	CHANGESPEED,
	STOP,


};


class Car : public Vehicle {

	std::string name;
	std::string color;
	long speed;

public:
	void SetSpeed(long s) {
		std::cout << "Car speed is: " << s << std::endl;;
		this->speed =s;
	}

	void  Play_music(){
		std::cout << "Music is On! " << std::endl;
	}


	void Adjust_Seats() {
		std::cout << "Seats are adjusted! " << std::endl;
	}




};






 using typestateVehicle = Typestate_Template<

	State<VehicleStates::IDEL, &Vehicle::SetSpeed, VehicleStates::START>,
	State<VehicleStates::START, &Vehicle::StopTheVehicle, VehicleStates::STOP>,
	State<VehicleStates::START, &Vehicle::SetSpeed, VehicleStates::CHANGESPEED>,
	State<VehicleStates::CHANGESPEED, &Vehicle::SetSpeed, VehicleStates::CHANGESPEED>,
	State<VehicleStates::CHANGESPEED, &Vehicle::StopTheVehicle, VehicleStates::STOP>

>  ;
  



#endif
 using vehicle = TypestateClassConnector<Vehicle, typestateVehicle>;
 using car = TypestateClassConnector<Car, typestateVehicle>;
 int main() {
	 
	 vehicle V1; 
	 car C;

	
	 // will compile it follows the typestates
	 (V1->* & Vehicle::SetSpeed)(70);
	 (V1->* &Vehicle::SetSpeed)(50);
   (V1->* &Vehicle::StopTheVehicle)();
  // not compile
  //(V1->* &Vehicle::SetSpeed)(70);
	 

// the subclass also adhere to the typestates

	 (C->* & Car::SetSpeed)(70);
	 (C->* & Car::Adjust_Seats)();
	 (C->* & Car::StopTheVehicle)();

	 // this is an example of subclass that does not follow the typestate
	 // it won't compilel
	  //(C->*&Car::StopTheVehicle)();
	  //(C->*&Car::SetSpeed)(70);



	 return 0;
 }
