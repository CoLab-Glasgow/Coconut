
#include "Car.h"

int main() {

	car V1;

	// this will compile 
	(V1->* & Car::SetSpeed)(50);
	(V1->* & Car::SetSpeed)(70);
	(V1->* & Car::SetSpeed)(50);
	(V1->* & Car::StopTheVehicle)();

	// this is will not compile
	//(V1->* & car::StopTheVehicle)();
	//(V1->* & car::SetSpeed)(70);

	return 0;
}
