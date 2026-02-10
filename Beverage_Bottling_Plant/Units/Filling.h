#include "MQTTProcessingUnit.h"

class  Filling: public ProcessingUnit {
    public:
    Filling(const std::string& broker, int port, const std::string& client_id)
            : ProcessingUnit(broker, port, client_id),  // Call base class constructor
              broker_(broker), port_(port), client_id_(client_id), ioc() {
        
            // Create synchronous client 
            client_ = MQTT_NS::make_tls_sync_client(ioc, broker_, std::to_string(port_));
        }
        bool is_Pasteurisation_complete() const {
            return Pasteurisation_complete.load();
        }

        
        void hotFilling() {
          std::cout << "Performing Hot Filling: Heating the drink and filling the container to kill germs." << std::endl;
            }
        void coldFillingAseptic() {
           std::cout << "Performing Cold Filling (Aseptic): Keeping the drink cool and filling it into sterilised containers." << std::endl;
             }

        void retortProcessingFilling() {
          std::cout << "Performing Retort Processing Filling: Heating the drink twice before filling for extra safety." << std::endl;
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
    
    enum FillingState {
        Ready=6, Heating, Mixing, filtering, Blending,  Complete
    };
    
    using Filling_Typestates = Typestate_Template<
      State<UStat::Subscribed, &Filling::is_Pasteurisation_complete, FillingState::Ready>,
        State<FillingState::Ready, &Filling::hotFilling, UStat::Processed>,
        State<FillingState::Ready, &Filling::coldFillingAseptic, UStat::Processed>, 
        State< FillingState::Ready, &Filling::retortProcessingFilling,  UStat::Processed>
    
    >;
    
    TypestateClassConnector<Filling, Filling_Typestates> Fillingflag;
    