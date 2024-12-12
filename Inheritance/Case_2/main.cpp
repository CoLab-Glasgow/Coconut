#include "Vehicle.h"
#include "Car.h"
 
 int main() {
	 Vehicle V1; 
	 Car C;
	 Vehicle& c_alise =C;
	 Vehicle& v_alis=V1;
	
	 v_alis.Start();
	 V1.Set_Speed(70);
	 V1.Set_Speed(50);
     V1.Stop();
       
     C.Start();
   //C.Set_Speed(70);
	 C.Play_Music();
	 c_alise.Set_Speed(30);
	 C.Adjust_Seats();
	 C.Stop();
	 return 0;
 }
