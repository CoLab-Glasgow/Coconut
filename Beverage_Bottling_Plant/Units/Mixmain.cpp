#include "Mixing.h"


void SetClient(ProcessingUnit& m)
{
    m.set_client_id("mixing_unit");
    m.set_clean_session(true);
    m.get_ssl_context("Beverage_Bottling_Plant/mixing.cert.pem", "Beverage_Bottling_Plant/mixing.key.pem");
    std::cout<<"is Configured and TLS-secured connection established with broker\n";
    m.connect();
    m.set_publish_handler();
    m.subscribe("water/topic", 1);

   

}
    

int main(){

  Mixing mixing("127.0.0.1", 8883, "mixing_unit");
  SetClient(mixing);

 
  

  std::thread io_thread([&mixing](){
      mixing.run_io_context();
  });
  
  std::cout << "Unit is running. Waiting for 'Clean water is ready' notification...\n";

  // Wait until the mixing unit's publish handler sets the flag.
  while (!mixing.is_water_ready()) {
      std::this_thread::sleep_for(std::chrono::milliseconds(50));
  }
  
  std::cout << "Water is ready received, Proceeding with mixing process...\n";

  // Continue with mixing operations
  mixing.prepare_formula();
  std::this_thread::sleep_for(std::chrono::seconds(5));
  mixing.regulate_quality_factors();
  std::this_thread::sleep_for(std::chrono::seconds(5));
  mixing.mix();
  std::this_thread::sleep_for(std::chrono::seconds(5));
 
  std::cout << "All processes complete. Publishing completion signal to Broker." << std::endl;
  std::this_thread::sleep_for(std::chrono::seconds(5));
  mixing.publish("Mixing/topic", "Mixing is complete", 1, true);
  
  std::cout << "Unit is running. Press Enter to exit...\n";
  std::cin.get();
  
  mixing.stop_io_context();
  io_thread.join();
  
  return 0;
}