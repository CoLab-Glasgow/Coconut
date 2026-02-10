#include "MQTTProcessingUnit.h"

class Mixing : public ProcessingUnit {
    public:
        Mixing(const std::string& broker, int port, const std::string& client_id)
            : ProcessingUnit(broker, port, client_id),  // Call base class constructor
              broker_(broker), port_(port), client_id_(client_id), ioc() {
        
            // Create synchronous client 
            client_ = MQTT_NS::make_tls_sync_client(ioc, broker_, std::to_string(port_));
        }
        bool is_water_ready() const {
            return water_ready.load();
        }
        
        void prepare_formula() {
            std::cout << "Preparing commercial formula..." << std::endl;
        }
        
        void regulate_quality_factors() {
            std::cout << "Regulating quality factors..." << std::endl;
        }
        
        void mix() {
            std::cout << "Mixing ingredients..." << std::endl;
        }
        
        void Finish() {

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
    
    enum MixingState {
         Ready=6, Preparing, Regulating
    };
    
    using Mixing_Typestates = Typestate_Template<
      State<UStat::Subscribed, &Mixing::is_water_ready,MixingState::Ready>,
        State<MixingState::Ready, &Mixing::prepare_formula, MixingState::Preparing>,
        State<MixingState::Preparing, &Mixing::regulate_quality_factors, MixingState::Regulating>, 
        State<MixingState::Regulating, &Mixing::mix, UStat::Processed>
    >;
    
    TypestateClassConnector<Mixing, Mixing_Typestates> Mixingflag;
    