#include "Car.h"

int main() {
	Car C1;
    Car& C2= C1;
	C1.Start();
	C1.Play_music();
//	C2.Start();
	C1.SetSpeed(50);
	C1.SetSpeed(70);
    C1.StopTheVehicle();
	return 0;
}
