
#ifndef Vehicle_HEADER
#define Vehicle_HEADER




#include <iostream>
// case 1 A class without typestate inherits from a class with typestate. 

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


#endif
