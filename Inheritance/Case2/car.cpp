#include <iostream>

#ifndef Vehicle_HEADER
#define Vehicle_HEADER


#include "C:\Users\Arwa Alsubhi\source\repos\Cocount_Typestate_based_tool\Cocount_Typestate_based_tool\TypestateLibrary.h"


using TypestateLibrary::TypestateClassConnector;
using TypestateLibrary::State;
using TypestateLibrary::Typestate_Template;





// case 2 :  A class with typestate inherits from a class without typestate.
class vehicle
{
	long speed;

public:
	void  Play_music() {
		std::cout << "Music is On! " << std::endl;
	}


	void Adjust_Seats() {
		std::cout << "Seats are adjusted! " << std::endl;
	}


};

class car : public vehicle {

	long speed;


public:

	void SetSpeed(long s) {
		this->speed = s;
	}

	void StopTheVehicle() {
		std::cout << "Vehicle Stopped !";

	}


	};

enum CarStates {
	IDEL,
	START,
	CHANGESPEED,
	STOP,


};



using typestatecar= Typestate_Template<

	State<CarStates::IDEL, &car::SetSpeed, CarStates::START>,
	State<CarStates::START, &car::StopTheVehicle, CarStates::STOP>,
	State<CarStates::START, &car::SetSpeed, CarStates::CHANGESPEED>,
	State<CarStates::CHANGESPEED, &car::SetSpeed, CarStates::CHANGESPEED>,
	State<CarStates::CHANGESPEED, &car::StopTheVehicle, CarStates::STOP>

>  ;


#endif
using Car = TypestateClassConnector<car, typestatecar>;
int main() {

	Car V1;

	// this will compile 
	(V1->* & car::SetSpeed)(50);
	(V1->* & car::SetSpeed)(70);
	(V1->* & car::SetSpeed)();
	(V1->* & car::StopTheVehicle)();

	// this is will not compile
	//(V1->* & car::StopTheVehicle)();
	//(V1->* & car::SetSpeed)(70);

	return 0;
}
