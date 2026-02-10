#include "MQTTProcessingUnit.h"

class WaterTreatment : public ProcessingUnit {
public:
    WaterTreatment(const std::string& broker, int port, const std::string& client_id)
        : ProcessingUnit(broker, port, client_id) {
        client_ = MQTT_NS::make_tls_sync_client(ioc, broker_, std::to_string(port_));
    }
    bool is_all_units_connected() const {
            return system_ready_.load();
        }
        

    void filter_water() {
        std::cout << "Filtering water..." << std::endl;
    }
    
    void disinfect_water() {
        std::cout << "Disinfecting water..." << std::endl;
    }
    
    void reverse_osmosis() {
        std::cout << "Performing reverse osmosis..." << std::endl;
    }
    
    void Finish() {
        std::cout << "Water treatment complete." << std::endl;    
    }
   

    private:
        std::string broker_;
        int port_;
        std::string client_id_;
    
        // client tls
        decltype(MQTT_NS::make_tls_sync_client(
            std::declval<boost::asio::io_context&>(), 
            std::declval<std::string>(), 
            std::declval<std::string>()
        )) client_;
    
        boost::asio::io_context ioc;  // Using direct `io_context` instead of `shared_ptr`
        std::thread io_thread_;
        bool cleanSession = false;
};

    

  using TypestateLibrary::Template::TypestateClassConnector;
    using TypestateLibrary::Template::State;
    using TypestateLibrary::Template::Typestate_Template;
    
    enum class WTStat {Ready=6,Filtered, Disinfected};

using WT_Typestates = Typestate_Template<
State<UStat::Subscribed, &WaterTreatment::is_all_units_connected,WTStat::Ready>,
State<WTStat::Ready,&WaterTreatment::filter_water,WTStat::Filtered>, 
State<WTStat::Filtered,&WaterTreatment::disinfect_water,WTStat::Disinfected>, 
State<WTStat::Disinfected,&WaterTreatment::reverse_osmosis,UStat::Processed>
>; 
    
    TypestateClassConnector<WaterTreatment, WT_Typestates> WTflag;
    