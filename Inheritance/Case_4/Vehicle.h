
#ifndef Vehicle_HEADER
#define Vehicle_HEADER

#include "../TypestateLibrary.h"

using TypestateLibrary::Template::State;
using TypestateLibrary::Template::Typestate_Template;
using TypestateLibrary::Template::TypestateClassConnector;

enum VehicleStates {
	IDEL=0,
	START=1,
	MOVEING=2,
	STOP=5
};

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


    
using typestateVehicle = Typestate_Template<
	State<VehicleStates::IDEL ,&Vehicle::Start ,VehicleStates::START>,
	State<VehicleStates::START , &Vehicle::Set_speed, VehicleStates::MOVEING>,
	State<VehicleStates::MOVEING , &Vehicle::Set_speed ,VehicleStates::MOVEING>,
	State<VehicleStates::MOVEING , &Vehicle::Stop, VehicleStates::STOP>>;


using vehicle_flag = TypestateClassConnector<Vehicle, typestateVehicle>;

vehicle_flag vflag;
typestateVehicle TypestV;
void init(){
TypestV.display();
vflag.display();
}

#endif