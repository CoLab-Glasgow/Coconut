#include "Vehicle.h"
#include "iostream"
#include "../TypestateLibrary.h"
#include "Vehicle.h"

using TypestateLibrary::Template::State;
using TypestateLibrary::Template::Typestate_Template;
using TypestateLibrary::Template::TypestateClassConnector;

class Car : public Vehicle {
public:
    void Activate_cruise(const long d) {
		std::cout<<"Activate cruise control with speed "<<d<<std::endl;
	}
	void  Pressing_Accelerator(long d) {
		std::cout << "Pressing Accelerator whith speed "<<d <<std::endl;
	}
	void Apply_brakes(long d) {
		std::cout << "Apply brakes with speed "<<d<<std::endl;
	}
	void Halt(){
		std::cout << "Stop the car" << std::endl;
	}	
};

enum CarStates{ MOVING=2, CRUISING=3, BRAKING=4 ,STOPPED=5} ;

using TypestateCar = Typestate_Template <
	State<CarStates::MOVING, &Car::Activate_cruise, CarStates::CRUISING>,
	State<CarStates::CRUISING, &Car::Apply_brakes, CarStates::BRAKING>,
	State<CarStates::MOVING, &Car::Apply_brakes, CarStates::BRAKING>,
	State<CarStates::CRUISING, &Car::Apply_brakes, CarStates::BRAKING>,
	State<CarStates::BRAKING, &Car::Halt, CarStates::STOPPED>
	>;

using car_flag = TypestateClassConnector<Car,TypestateCar>;

car_flag carflag;
TypestateCar Tcar;
void init_car(){
Tcar.display();
carflag.display();
}