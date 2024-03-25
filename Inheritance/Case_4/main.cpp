#include "Vehicle.h"
#include "Car.h"
#include "TypestateLibrary.h"

static void operateVehicle (vehicle& v) {
    (v->*&Vehicle::Start)();
    (v->*&Vehicle::Set_speed)(70);
    (v->*&Vehicle::Stop)();
}

int main() {
    car car;
    vehicle vehicle;
    operateVehicle(vehicle);
    operateVehicle(car);
    (car->*&Car::Activate_cruise)(50);
    (car->*&Car::Apply_brakes)(20);
    (car->*&Car::Halt)();
    return 0;
 }
