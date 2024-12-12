
#include "Vehicle.h"
#include "Car.h"

 void operateVehicle(Vehicle& v) {
    v.Start();
    v.Set_speed(70);
    v.Stop();
}

int main() {
   
    Vehicle vehicle;
    Car car;
    operateVehicle(vehicle);
    operateVehicle(car);
   
   
    return 0;
 }
