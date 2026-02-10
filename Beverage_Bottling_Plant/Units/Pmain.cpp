#include "Pasteurisation.h"



void SetClient(ProcessingUnit& m)
{
    m.set_client_id("Pasteurisation");
    m.set_clean_session(true);
    m.get_ssl_context("Beverage_Bottling_Plant/Pasteurisation.crt.pem", "Beverage_Bottling_Plant/Pasteurisation.key.pem");
    std::cout<<"is Configured and TLS-secured connection established with broker\n";
     m.connect();
    m.set_publish_handler();
    m.subscribe("homogenisation/topic", 1);


}
    

int main(){

   
    Pasteurisation pasteurisation("127.0.0.1", 8883, "Pasteurisation");

   
   

 
    SetClient(pasteurisation);

  std::thread io_thread([&pasteurisation](){
    pasteurisation.run_io_context();
  });
 
  
  std::cout << "Unit is running. Waiting for 'Homogenisation is complete' notification...\n";

 
  while (!pasteurisation.is_homogenisation_complete()) {
      std::this_thread::sleep_for(std::chrono::milliseconds(50));
  }
  
  std::cout << "'Homogenisation is Complete' received. Proceeding with Pasteurisation process...\n";

   pasteurisation.pasteurise_retort();
   std::this_thread::sleep_for(std::chrono::seconds(5));

   std::cout << "All processes complete. Publishing completion signal to Broker." << std::endl;
   std::this_thread::sleep_for(std::chrono::seconds(5));
   pasteurisation.publish("Pasteurisation/topic", "Pasteurisation is complete", 1, true);
  
   std::cout << "Unit is running. Press Enter to exit...\n";
   std::cin.get();
  
  pasteurisation.stop_io_context();
  io_thread.join();
   

    return 0;

    
}