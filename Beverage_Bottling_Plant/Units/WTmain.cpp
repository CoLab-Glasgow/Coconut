#include "WaterTreatment.h"
#include <thread>
#include <chrono>
#include <iostream>

static void SetClient(ProcessingUnit& m)
{
    m.set_client_id("water_unit");
    m.set_clean_session(true);
    m.get_ssl_context("Beverage_Bottling_Plant/water_unit.crt.pem",
                      "Beverage_Bottling_Plant/water_unit.key.pem");
    std::cout << "Configured\n";
  
    m.connect();
    m.set_publish_handler(); 
    m.subscribe("system/ready", 1);
}

int main() {
    WaterTreatment water_treatment("127.0.0.1", 8883, "water_unit");

    SetClient(water_treatment);

    // Run the io_context ( class starts its own thread)
    water_treatment.run_io_context();

    std::cout << "[Water Unit] Unit is running. Waiting for system ready notification...\n";
    while (!water_treatment.is_all_units_connected()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
    std::cout << "[Water Unit] System ready received. Proceeding with water unit processing...\n";

   
    water_treatment.filter_water();
    std::this_thread::sleep_for(std::chrono::seconds(5));
   
    water_treatment.disinfect_water();
    std::this_thread::sleep_for(std::chrono::seconds(5));
 
    water_treatment.reverse_osmosis();
    std::this_thread::sleep_for(std::chrono::seconds(5));

    
    water_treatment.publish("water/topic", "Clean Water is Ready", /*qos=*1, /*retain=*/true);

    
    std::cout << "[Water_Unit] Publish water/topic With payload: Clean Water is Ready\n";

    std::cin.get();

    water_treatment.stop_io_context();
    return 0;
}
