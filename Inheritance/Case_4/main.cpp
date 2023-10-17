
#include "Vehicle.h"
#include "Car.h"
#include "TypestateLibrary.h"

static void operateVehicle (Vehicle& v) {
    (v->*&Vehicle::Start)();
    (v->*&Vehicle::Set_Speed)(70);
    (v->*&Vehicle::Stop)();
}

int main() {
    Car car;
    Vehicle vehicle;
    operateVehicle(vehicle);
    operateVehicle(car);
    (car->*&Car::Activate_cruise)(50);
    (car->*&Car::Apply_brakes)(20);
    (car->*&Car::Halt)();
    return 0;
 }
