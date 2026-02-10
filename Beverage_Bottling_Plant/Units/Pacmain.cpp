#include "Packaging.h"



void SetClient(ProcessingUnit& m)
{
    
   

    m.set_client_id("Packaging");
    m.set_clean_session(true);
    m.get_ssl_context("Beverage_Bottling_Plant/Packaging.crt.pem", "Beverage_Bottling_Plant/Packaging.key.pem");
    std::cout<<"is Configured and TLS-secured connection established with broker\n";
    m.connect();

}
    

int main(){

   
    Packaging Packager("127.0.0.1", 8883, "Packaging");

   
   SetClient(Packager);

   
   Packager.set_publish_handler();
   Packager.subscribe("Filling/topic", 1);

  std::thread io_thread([&Packager](){
    Packager.run_io_context();
  });
  
  std::cout << "Unit is running. Waiting for 'Filling is complete' notification...\n";

 
  while (!Packager.is_Filling_complete()) {
      std::this_thread::sleep_for(std::chrono::milliseconds(50));
  }
  
  std::cout << "'Filling is Complete' Proceeding with Packaging process...\n";

   Packager.labelProduct();
   std::this_thread::sleep_for(std::chrono::seconds(5));
   Packager.packProduct();
   std::this_thread::sleep_for(std::chrono::seconds(5));
   
   

  std::cout << "Packaging unit is running. Press Enter to exit...\n";
  std::cin.get();
  
  Packager.stop_io_context();
  io_thread.join();
   

    return 0;

    
}