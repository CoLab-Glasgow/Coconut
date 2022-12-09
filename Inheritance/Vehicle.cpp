#include "Vehicle.h"
#include <iostream>
#include "Typestate.h"


class Vehicle
{
	long speed;

public:
	void SetSpeed(long s) {
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



using TypestateTool::State;
using TypestateTool::typestate;



using VehicleTypestate = typestate<
  
	State<VehicleStates::IDEL, &Vehicle::SetSpeed, VehicleStates::START>,
	State<VehicleStates::START, &Vehicle::StopTheVehicle, VehicleStates::STOP>,
	State<VehicleStates::START, &Vehicle::SetSpeed, VehicleStates::CHANGESPEED>,
	State<VehicleStates::CHANGESPEED, &Vehicle::SetSpeed, VehicleStates::CHANGESPEED>,
	State<VehicleStates::CHANGESPEED, &Vehicle::StopTheVehicle, VehicleStates::STOP>,

>;

