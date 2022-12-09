#include "Car.h"
#include "Vehicle.h"
#include "Vehicle.cpp"

class car : public Vehicle {

	string name;
	string date;

public:

	void SetCarName(string n) {
		this->name = n;
	}

  
	void SetCarDateRelease(string d) {
		this->date = d;
	}





};


