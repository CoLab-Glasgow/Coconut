#include "Filling.h"



void SetClient(ProcessingUnit& m)
{
    
   

    m.set_client_id("Filling");
    m.set_clean_session(true);
    m.get_ssl_context("Beverage_Bottling_Plant/Filling.crt.pem", "Beverage_Bottling_Plant/Filling.key.pem");
    std::cout<<"is Configured and TLS-secured connection established with broker\n";
    m.connect();
   // m.process();

}
    

int main(){

   
    Filling filler("127.0.0.1", 8883, "Filling");

   
   SetClient(filler);


   filler.set_publish_handler();
   filler.subscribe("Pasteurisation/topic", 1);

  std::thread io_thread([&filler](){
    filler.run_io_context();
  });
  
  std::cout << "Unit is running. Waiting for 'Pasteurisation is complete' notification...\n";

 
  while (!filler.is_Pasteurisation_complete()) {
      std::this_thread::sleep_for(std::chrono::milliseconds(50));
  }
  
  std::cout << "Received notification 'Pasteurisation is Complete!' Proceeding with Filling process...\n";

  
    filler.coldFillingAseptic();
    std::this_thread::sleep_for(std::chrono::seconds(5));
  
    std::cout << "All processes complete. Publishing completion signal to Broker." << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(5));
    
  filler.publish("Filling/topic", "Filling is complete", 1, true);
  
  std::cout << "Unit is running. Press Enter to exit...\n";
  std::cin.get();
 
  filler.stop_io_context();
  io_thread.join();
   

    return 0;

    
}