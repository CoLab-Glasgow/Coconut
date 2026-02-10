#include "Homogenisation.h"



void SetClient(ProcessingUnit& m)
{
    
    m.set_client_id("Homogenisation");
    m.set_clean_session(true);
    m.get_ssl_context("Beverage_Bottling_Plant/homogenisation.crt.pem", "Beverage_Bottling_Plant/homogenisation.key.pem");
    std::cout<<"is Configured and TLS-secured connection established with broker\n";
    m.connect();
  
}
    

int main(){

   
    Homogenisation homogenisation("127.0.0.1", 8883, "Homogenisation");

   
   SetClient(homogenisation);


   homogenisation.set_publish_handler();
   homogenisation.subscribe("Mixing/topic", 1);

  std::thread io_thread([&homogenisation](){
    homogenisation.run_io_context();
  });
  
  std::cout << "Unit is running. Waiting for 'Mixing is complete' notification...\n";

 
  while (!homogenisation.is_Mixing_ready()) {
      std::this_thread::sleep_for(std::chrono::milliseconds(50));
  }
  
  std::cout << "'Mixing is Complete' received, Proceeding with homogenisation process...\n";

   
   homogenisation.heat_up_mixing_tank();
   std::this_thread::sleep_for(std::chrono::seconds(5));
   homogenisation.dissolve_granular_ingredients();
   std::this_thread::sleep_for(std::chrono::seconds(5));
   homogenisation.filter_oversized_particles();
   std::this_thread::sleep_for(std::chrono::seconds(5));
   homogenisation.blend_up();
   std::this_thread::sleep_for(std::chrono::seconds(5));
   homogenisation.Finish();
   std::this_thread::sleep_for(std::chrono::seconds(5));
   
   
   std::cout << "All processes complete. Publishing completion signal to Broker." << std::endl;
   std::this_thread::sleep_for(std::chrono::seconds(5));
    
   
   homogenisation.publish("homogenisation/topic", "Homogenisation is complete", 1, true);
  
   std::cout << "Unit is running. Press Enter to exit...\n";
   std::cin.get();
  
  homogenisation.stop_io_context();
  io_thread.join();
   

    return 0;

    
}