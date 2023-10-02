#include <iostream>

#ifndef Vehicle_HEADER
#define Vehicle_HEADER


#include "TypestateLibrary.h"
#include "Vehicle.h"
using TypestateLibrary::TypestateClassConnector;
using TypestateLibrary::State;
using TypestateLibrary::Typestate_Template;



class Car : public Vehicle {

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

	State<CarStates::IDEL, &Car::SetSpeed, CarStates::START>,
	State<CarStates::START, &Car::StopTheVehicle, CarStates::STOP>,
	State<CarStates::START, &Car::SetSpeed, CarStates::CHANGESPEED>,
	State<CarStates::CHANGESPEED, &Car::SetSpeed, CarStates::CHANGESPEED>,
	State<CarStates::CHANGESPEED, &Car::StopTheVehicle, CarStates::STOP>

>  ;


#endif
using car = TypestateClassConnector<Car, typestatecar>;

