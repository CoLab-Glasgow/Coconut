#ifndef Vehicle_HEADER
#define Vehicle_HEADER

#include "../include/Typestate_Library/TypestateLibrary.h"

using TypestateLibrary::Template::State;
using TypestateLibrary::Template::Typestate_Template;
using TypestateLibrary::Template::TypestateClassConnector;

enum VehicleStates {
	IDEL,
	START,
	MOVEING,
	STOP
};
// case 2: a class without typestate inherits from a class with typestate. 
class Vehicle
{
	long speed;
public:
	void Start(){
		std::cout << "Starting the Vehicle" << std::endl;
	}
	void Set_Speed(long s) {
		std::cout << "Vehicle speed is: " << s << std::endl;;
		this->speed = s;
	}
        void Stop() {
		std::cout << "Vehicle Stopped !";
	}
	
};





using Vehicle_Typestate = Typestate_Template<
    State<VehicleStates::IDEL, &Vehicle::Start, VehicleStates::START>,
	State<VehicleStates::START, &Vehicle::Set_Speed, VehicleStates::MOVEING>,
	State<VehicleStates::MOVEING, &Vehicle::Set_Speed, VehicleStates::MOVEING>,
	State<VehicleStates::MOVEING, &Vehicle::Stop, VehicleStates::STOP>>;


using Vehicle_flag = TypestateClassConnector<Vehicle, Vehicle_Typestate>;

Vehicle_flag VehicleFlag;
Vehicle_Typestate VT;
void init(){
VT.display();
VehicleFlag.display();
}



#endif