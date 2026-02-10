#include "MQTTProcessingUnit.h"

class  Homogenisation: public ProcessingUnit {
    public:
    Homogenisation(const std::string& broker, int port, const std::string& client_id)
            : ProcessingUnit(broker, port, client_id),  // Call base class constructor
              broker_(broker), port_(port), client_id_(client_id), ioc() {
        
            // Create synchronous client 
            client_ = MQTT_NS::make_tls_sync_client(ioc, broker_, std::to_string(port_));
        }
        bool is_Mixing_ready() const {
            return Mixing_ready.load();
        }
        void heat_up_mixing_tank() {
            std::cout << "Heating up mixing tank..." << std::endl;
        }
        
        void dissolve_granular_ingredients() {
            std::cout << "Dissolving granular ingredients..." << std::endl;
        }
        
        void filter_oversized_particles() {
            std::cout << "Filtering oversized particles..." << std::endl;
        }
        
        void blend_up() {
            std::cout << "Blending ingredients..." << std::endl;
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
    
    enum HomogenisationState {
        Ready=6, Heating, Mixing, filtering, Blending,  Complete
    };
    
    using Homogenisation_Typestates = Typestate_Template<
    State<UStat::Subscribed, &Homogenisation::is_Mixing_ready, HomogenisationState::Ready>,
        State<HomogenisationState::Ready, &Homogenisation::heat_up_mixing_tank, HomogenisationState::Heating>,
        State<HomogenisationState::Heating, &Homogenisation::dissolve_granular_ingredients, HomogenisationState::Mixing>, 
        State<HomogenisationState::Mixing, &Homogenisation::filter_oversized_particles, HomogenisationState::filtering>,
        State<HomogenisationState::filtering, &Homogenisation::blend_up, UStat::Processed>
        


    >;
    
    TypestateClassConnector<Homogenisation, Homogenisation_Typestates> Homogenisationflag;
    