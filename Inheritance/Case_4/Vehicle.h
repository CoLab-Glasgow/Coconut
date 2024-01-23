#include "TypestateLibrary.h"
#ifndef Vehicle_HEADER
#define Vehicle_HEADER

using TypestateLibrary::State;
using TypestateLibrary::TypestateClassConnector;
using TypestateLibrary::Typestate_Template;


class Vehicle
{
	long speed;

public:
	void Start() {
		std::cout << "Turns the ignition key!"<<std::endl;
	}
	void Set_speed(long s) {
		std::cout<<"Set speed to "<< s <<std::endl;
	}
	void Stop() {
		std::cout << "Turning off the engine" << std::endl;
	}

};


BETTER_ENUM (VehicleStates, int, IDEL, START,  MOVING, STOPPED);
          
using typestateVehicle = Typestate_Template<
	State<+VehicleStates::IDEL ,&Vehicle::Start, +VehicleStates::START>,
	State<+VehicleStates::START , &Vehicle::Set_speed, +VehicleStates::MOVING>,
	State<+VehicleStates::MOVING , &Vehicle::Set_speed, +VehicleStates::MOVING>,
	State<+VehicleStates::MOVING , &Vehicle::Stop, +VehicleStates::STOPPED>>;

#endif
using vehicle = TypestateClassConnector<Vehicle, typestateVehicle>;
