#include <mqtt_client_cpp.hpp>
#include <boost/asio.hpp>
#include <iostream>
#include <memory>
#include <functional>
#include <thread>

#include "../../include/Typestate_Library/TypestateLibrary.h"

class ProcessingUnit {
    public:
    ProcessingUnit(const std::string& broker, int port, const std::string& client_id)
    : broker_(broker)
    , port_(port)
    , client_id_(client_id)
    , ioc()
    , system_ready_{false}   // Initialize to false
    , terminated_{false}     // Initialize to false if used
{
    // Create synchronous client 
    client_ = MQTT_NS::make_tls_sync_client(ioc, broker_, std::to_string(port_));
   
            // Define the correct type for packet ID
            using packet_id_t = typename std::remove_reference_t<decltype(*client_)>::packet_id_t;
            std::cout << "Initialised with Broker: " << broker
            << " Port: " << port << " ClientID: " << client_id << std::endl;
            
            }
        void set_client_id(std::string id){
            client_->set_client_id(id);

        }
        void set_clean_session(bool clean){
           client_->set_clean_session(clean);
        }
        void get_ssl_context(std::string c, std::string key){
        
        client_->get_ssl_context().load_verify_file("Beverage_Bottling_Plant/cacert.pem");
        //client_->get_ssl_context().use_certificate_file(c, boost::asio::ssl::context::pem);
       // client_->get_ssl_context().use_private_key_file(key, boost::asio::ssl::context::pem);
            
      
        }
       
        void connect() {

            
            // Start the connection process.
            client_->connect();
            
        
            
        }
        
       
        void setup_system_ready_handler() {
            client_->set_publish_handler(
                [this](boost::optional<unsigned short> packet_id,
                       mqtt::publish_options pubopts,
                       mqtt::buffer topic,
                       mqtt::buffer contents) -> bool {
                    std::string topic_str(reinterpret_cast<const char*>(topic.data()), topic.size());
                    std::string payload(reinterpret_cast<const char*>(contents.data()), contents.size());
                    std::cout << "Received message on topic: " 
                              << topic_str << " with payload: " << payload << "\n";
                    // If the message on "system/ready" is received, set the flag.
                    if (topic_str == "system/ready") {
                        std::cout << "'System ready' notification received.\n";
                        system_ready_.store(true);
                    }
                    return true;
                }
            );
        }
    
        void publish(const std::string& topic, const std::string& message, int qos = 1, bool retain = false) {
            std::cout<<"Publish "<< topic<< "With payload: "<< message <<"\n";
            client_->publish(
                mqtt::allocate_buffer(topic), 
                mqtt::allocate_buffer(message), 
                mqtt::qos(qos), 
                mqtt::retain(retain)
            );
        
            
        }
        void subscribe(const std::string& topic, int qos = 1) {
            client_->subscribe(mqtt::allocate_buffer(topic), mqtt::qos(qos));
            std::cout << "Subscribed to " << topic << " with QoS: " << qos << "\n";
        }

        void set_publish_handler() {
            client_->set_publish_handler(
                [this](boost::optional<unsigned short> packet_id,
                       mqtt::publish_options pubopts,
                       mqtt::buffer topic,
                       mqtt::buffer contents) -> bool {
                    // Convert the topic and contents to std::string.
                    std::string topic_str(reinterpret_cast<const char*>(topic.data()), topic.size());
                    std::string payload(reinterpret_cast<const char*>(contents.data()), contents.size());
                    
                    std::cout << "Received message on topic: " 
                              << topic_str << " with payload: " << payload << "\n";
                   if (topic_str == "system/ready" && payload == "ok") {
                       std::cout << "'system/ready' notification received.\n";
                        system_ready_.store(true);
                    }
                    
                    if (topic_str == "water/topic" && payload == "Clean Water is Ready") {
                      
                        water_ready.store(true);
                    }
                    if (topic_str == "Mixing/topic" && payload == "Mixing is complete"){
                        
                        Mixing_ready.store(true);
                    }
                    if (topic_str == "homogenisation/topic" && payload == "Homogenisation is complete"){
                       
                        homogenisation_complete.store(true);
                    }
                    if (topic_str == "Pasteurisation/topic" && payload == "Pasteurisation is complete"){
                        
                        Pasteurisation_complete.store(true);
                    }
                    if (topic_str == "Filling/topic" && payload == "Filling is complete"){
                        
                        Filling_complete.store(true);
                    }
                    return true;
                }
            );
        }
        
        // Register the subscribe acknowledgment handler.
        // The handler receives a packet id and a vector of suback return codes.
        void set_subscribe_handler() {
            client_->set_suback_handler(
                [this](unsigned short packet_id, 
                       std::vector<mqtt::suback_return_code> results) -> bool {
                    std::cout << "Subscription acknowledged, packet id: " 
                              << packet_id << " Results: ";
                    for (auto e : results)
                        std::cout << static_cast<int>(e) << " ";
                    std::cout << "\n";
                    return true;
                }
            );
        }
        
        void disconnect() {
            client_->disconnect();
            std::cout << "Disconnected from MQTT broker" << std::endl;
        }

        void process(){

        }
    
        void run_io_context() {
            io_thread_ = std::thread([this] {
                ioc.run();
            });
        }
        bool is_terminated() const {
            return terminated_.load();
        }
        void stop_io_context() {
            ioc.stop();
            if (io_thread_.joinable()) {
                io_thread_.join();
            }
        }
void set_puback_handler() {
    using packet_id_t = typename std::remove_reference_t<decltype(*client_)>::packet_id_t;
    client_->set_puback_handler(
        [this](packet_id_t /*packet_id*/) {
            last_pub_acked.store(true);
            std::cout << "[Water_Unit] PUBACK received.\n";
            return true;
        }
    );
}
  std::atomic<bool> last_pub_acked{false};

    protected:
    decltype(MQTT_NS::make_tls_sync_client(
            std::declval<boost::asio::io_context&>(), 
            std::declval<std::string>(), 
            std::declval<std::string>()
        )) client_;
  
        std::string broker_;
        int port_;
        std::string client_id_;
        boost::asio::io_context ioc;  // Using direct `io_context` instead of `shared_ptr`
        std::thread io_thread_;
        bool cleanSession=false;
        
      
        std::atomic<bool> terminated_{false};
        std::atomic<bool> system_ready_{false};
        std::atomic<bool> water_ready{false};
        std::atomic<bool> Mixing_ready{false};
        std::atomic<bool> homogenisation_complete{false};
        std::atomic<bool> Pasteurisation_complete{false};
        std::atomic<bool> Filling_complete{false};
    };


    using TypestateLibrary::Template::TypestateClassConnector;
    using TypestateLibrary::Template::State;
    using TypestateLibrary::Template::Typestate_Template;
    
    enum class UStat {
Idle, Assigned, Configured, Connected, HandlerSet, Subscribed, Processed=9 , Published
};
using Client_Typestates = Typestate_Template<
State<UStat::Idle, &ProcessingUnit::set_client_id, UStat::Assigned>,
State<UStat::Assigned, &ProcessingUnit::get_ssl_context, UStat::Configured>,
State<UStat::Configured, &ProcessingUnit::connect, UStat::Connected>,
//State<UStat::Connected, &ProcessingUnit::set_publish_handler, UStat::HandlerSet>,
State<UStat::Connected, &ProcessingUnit::subscribe, UStat::Subscribed>,
State<UStat::Subscribed, &ProcessingUnit::process, UStat::Processed>, 
State<UStat::Processed, &ProcessingUnit::publish, UStat::Published>
>;   
    
    TypestateClassConnector<ProcessingUnit, Client_Typestates> Clientflag;
    