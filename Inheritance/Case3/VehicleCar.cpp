

#ifndef Vehicle_HEADER
#define Vehicle_HEADER





#include "C:\Users\Arwa Alsubhi\source\repos\Cocount_Typestate_based_tool\Cocount_Typestate_based_tool\TypestateLibrary.h"


using TypestateLibrary::State;
using TypestateLibrary::TypestateClassConnector;
using TypestateLibrary::Typestate_Template;
using TypestateLibrary::Merged_Typestate;
using TypestateLibrary::Subtyping_Checker;

// case 1 A class without typestate inherits from a class with typestate. 

class Vehicle
{
	long speed;

public:
	void turns_ignition_key() {
		std::cout << "Turns the ignition key! " << std::endl;
	}

	void pressing_accelerator(long s) {
		std::cout << "Pressing the accelerator with speed "<< s <<std::endl;

	}
	void turning_off() {
		std::cout << "Turning off the engine" << std::endl;
	}



};



BETTER_ENUM(VehicleStates, int, Off,ON, Driving, STOP)





class Car : public Vehicle {

	std::string name;
	std::string color;

public:

	void  shift_gears(long d) {
		std::cout << "shift gears whith speed " << d <<std::endl;
	}


	void apply_brakes() {
		std::cout << "Apply brakes to stop! " << std::endl;
	}
	void activate_cruise() {
		std::cout << "Activate cruise control!" << std::endl;
	}
};

BETTER_ENUM(CarStates, int , Driving, Accelerating, Braking , Cruising, Stop)






using typestateVehicle = Typestate_Template<

	State<VehicleStates::Off ,&Vehicle::turns_ignition_key ,VehicleStates::ON>,
	State<VehicleStates::ON , &Vehicle::pressing_accelerator, VehicleStates::Driving>,
	State<VehicleStates::Driving , &Vehicle::pressing_accelerator ,VehicleStates::Driving>,
	State<VehicleStates::Driving , &Vehicle::turning_off, VehicleStates::STOP> 
>  ;



using TypestateCar = Typestate_Template <
	State<VehicleStates::Driving, &Car::shift_gears, CarStates::Accelerating>,
	State<CarStates::Accelerating, &Car::activate_cruise, CarStates::Cruising>,
	State<CarStates::Accelerating, &Car::apply_brakes, CarStates::Braking>,
	State<CarStates::Cruising, &Car::apply_brakes, CarStates::Braking>,
	State<CarStates::Braking, &Car::turning_off, CarStates::Stop> >;



#endif

using MergedTypestate = Merged_Typestate<typestateVehicle, TypestateCar> ;
using vehicle = TypestateClassConnector <Vehicle, typestateVehicle> ;
using car = Subtyping_Checker<Car, MergedTypestate>;

int main() {
	
	 
	Subtyping_Checker<Car, MergedTypestate> ;
	
	

	
	vehicle V1;
	// will compile it follows the typestates
	(V1->* &Vehicle::turns_ignition_key)();
	(V1->* &Vehicle::pressing_accelerator)(50);
	(V1->* &Vehicle::pressing_accelerator)(100);
	(V1->* &Vehicle::turning_off)();
		//(V1->* &Vehicle::SetSpeed)(70);
	car car1;

	// the subclass also adhere to the typestates
    (car1->* & Vehicle::turns_ignition_key)();
    (car1->* & Vehicle::pressing_accelerator)(80);
    (car1->* & Car::shift_gears)(40);
    (car1->*& Car::apply_brakes)();
    (car1->* & Car::turning_off)();



	return 0;
}
