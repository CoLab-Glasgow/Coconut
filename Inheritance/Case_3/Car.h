
#include "../TypestateLibrary.h"
#include "Vehicle.h"

using TypestateLibrary::Template::State;
using TypestateLibrary::Template::Typestate_Template;
using TypestateLibrary::Template::TypestateClassConnector;

class Car : public Vehicle {

long speed;
public:
   void Start() {
		std::cout << "Turns the ignition key!"<<std::endl;
	}
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
	State<CarStates::IDEL, &Car::Start, CarStates::START>,
	State<CarStates::START, &Car::SetSpeed, CarStates::CHANGESPEED>,
	State<CarStates::CHANGESPEED, &Car::SetSpeed, CarStates::CHANGESPEED>,
	State<CarStates::CHANGESPEED, &Car::StopTheVehicle, CarStates::STOP>>;


using car_flag = TypestateClassConnector<Car, typestatecar>;

car_flag carflag;
typestatecar TC;
void init(){
TC.display();
carflag.display();
}
