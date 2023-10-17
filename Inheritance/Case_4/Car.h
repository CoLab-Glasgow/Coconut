#include "Vehicle.h"
#include "iostream"
#include "TypestateLibrary.h"

using TypestateLibrary::State;
using TypestateLibrary::TypestateClassConnector;
using TypestateLibrary::Typestate_Template;

class Car : public Vehicle {
public:
    void Activate_cruise(const long d) {
		std::cout<<"Activate cruise control with speed"<<d<<std::endl;
	}
	void  Pressing_Accelerator(long d) {
		std::cout << "Pressing Accelerator whith speed "<<d <<std::endl;
	}
	void Apply_brakes(long d) {
		std::cout << "Apply brakes with speed"<<d<<std::endl;
	}
	void Halt(){
		std::cout << "Stop the car" << std::endl;

	}
	
};

BETTER_ENUM(CarStates, int , MOVING, ACCELERATING, BRAKING , CRUISING, STOPPED);

using TypestateCar = Typestate_Template <
	State<CarStates::MOVING, &Car::Activate_cruise, CarStates::CRUISING>,
	State<CarStates::CRUISING, &Car::Pressing_Accelerator, CarStates::ACCELERATING>,
	State<CarStates::CRUISING, &Car::Apply_brakes, CarStates::BRAKING>,
	State<CarStates::MOVING, &Car::Apply_brakes, CarStates::BRAKING>,
	State<CarStates::BRAKING, &Car::Halt, CarStates::STOPPED>
	>;

using car = TypestateClassConnector<Car,TypestateCar>;