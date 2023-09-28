#include "Vehicle.h"

class Car : public vehicle {

	std::string name;
	
	long speed;

public:
	

	void  Play_Music(){
		std::cout << "Music is On! " << std::endl;
	}


	void Adjust_Seats() {
		std::cout << "Seats are adjusted! " << std::endl;
	}
	void Halt() {
		std::cout << "The car is stopped" << std::endl;

	}




};