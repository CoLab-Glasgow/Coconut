#include "MQTTProcessingUnit.h"

class  Pasteurisation: public ProcessingUnit {
    public:
    Pasteurisation(const std::string& broker, int port, const std::string& client_id)
            : ProcessingUnit(broker, port, client_id),  // Call base class constructor
              broker_(broker), port_(port), client_id_(client_id), ioc() {
        
            // Create synchronous client 
            client_ = MQTT_NS::make_tls_sync_client(ioc, broker_, std::to_string(port_));
        }
        bool is_homogenisation_complete() const {
            return homogenisation_complete.load();
        }
        void pasteurise_UHT(int temperature = 140, const std::string& duration = "5-6s") {
            std::cout << "UHT pasteurisation at " << temperature << "°C for " << duration << std::endl;
        }
        
        
        void pasteurise_HTST(int temperature = 72, const std::string& duration = "20s") {
            std::cout << "HTST pasteurisation at " << temperature << "°C for " << duration << std::endl;
        }
        
       
        void pasteurise_tunnel(int temperature = 60, const std::string& duration = "8min") {
            std::cout << "Tunnel pasteurisation at " << temperature << "°C for " << duration << std::endl;
        }
        
       
        void pasteurise_retort() {
            std::cout << "Retort pasteurisation using steam/pressure in a sealed tank" << std::endl;
        }
        

       void pasteurise_HPP() {
        std::cout << "HPP pasteurisation using high pressure without heat" << std::endl;
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
    
    enum PasteurisationState {
       Ready=6,  Complete=8
    };
    
    using Pasteurisation_Typestates = Typestate_Template<
     State<UStat::Subscribed, &Pasteurisation::is_homogenisation_complete, PasteurisationState::Ready>,
        State<PasteurisationState::Ready, &Pasteurisation::pasteurise_HPP, UStat::Processed>,
        State<PasteurisationState::Ready, &Pasteurisation::pasteurise_HTST, UStat::Processed>, 
        State<PasteurisationState::Ready, &Pasteurisation::pasteurise_retort, UStat::Processed>,
        State<PasteurisationState::Ready, &Pasteurisation::pasteurise_tunnel,UStat::Processed>, 
        State<PasteurisationState::Ready, &Pasteurisation::pasteurise_UHT, UStat::Processed> 


    >;
    
    TypestateClassConnector<Pasteurisation, Pasteurisation_Typestates> Pasteurisationflag;
    