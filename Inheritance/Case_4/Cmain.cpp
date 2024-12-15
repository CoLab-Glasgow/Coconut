#include "Car.h"
#include "Vehicle.h"

void Opreat(Vehicle& v)
{
v.Start();
v.Set_speed(98);

}
       
    
int main(){

Car car;
Opreat(car);
car.Activate_cruise(100); 
car.Stop();

return 0;
}
