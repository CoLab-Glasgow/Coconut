#ifndef Vehicle_HEADER
#define Vehicle_HEADER
#include "TypestateLibrary.h"
using TypestateLibrary::TypestateClassConnector;
using TypestateLibrary::State;
using TypestateLibrary::Typestate_Template;
// case 2: a class without typestate inherits from a class with typestate. 
class Vehicle
{
	long speed;
public:
	void Start() {
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


BETTER_ENUM (VehicleStates , int , 
	IDEL,
	START,
	MOVEING,
	STOP
)

using Vehicle_Typestate = Typestate_Template<
         State<VehicleStates::IDEL, &Vehicle::Start, VehicleStates::START>,
	State<VehicleStates::START, &Vehicle::Set_Speed, VehicleStates::MOVEING>,
	State<VehicleStates::MOVEING, &Vehicle::Set_Speed, VehicleStates::MOVEING>,
	State<VehicleStates::MOVEING, &Vehicle::Stop, VehicleStates::STOP>>;
using vehicle = TypestateClassConnector<Vehicle, Vehicle_Typestate>;
#endif
