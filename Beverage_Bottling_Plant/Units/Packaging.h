#include "MQTTProcessingUnit.h"

class Packaging: public ProcessingUnit {
    public:
    Packaging(const std::string& broker, int port, const std::string& client_id)
            : ProcessingUnit(broker, port, client_id),  // Call base class constructor
              broker_(broker), port_(port), client_id_(client_id), ioc() {
        
            // Create synchronous client 
            client_ = MQTT_NS::make_tls_sync_client(ioc, broker_, std::to_string(port_));
        }
        bool is_Filling_complete() const {
            return Filling_complete.load();
        }

        void labelProduct() {
            std::cout << "Applying labels..." << std::endl;
        }
    
        void packProduct() {
            std::cout << "Assembling packages..." << std::endl;
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
    
    enum PackagingState {
        Ready=6, Labeling, Mixing, filtering, Blending,  Complete
    };
    
    using Packaging_Typestates = Typestate_Template<
        State<UStat::Subscribed, &Packaging::is_Filling_complete, PackagingState::Ready>,
        State<PackagingState::Ready, &Packaging::labelProduct, PackagingState::Labeling>,
          State<PackagingState::Labeling, &Packaging::packProduct, UStat::Processed>
    >;
    
    TypestateClassConnector<Packaging, Packaging_Typestates> Packagingflag;
    