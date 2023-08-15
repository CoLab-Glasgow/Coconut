#pragma once
#include <iostream>
#include <fstream>
#include <sstream>
#include <better_enums/better_enums.hpp>

#include <functional>
#include <map>
#include <unordered_set>
#include <functional>
#include <assert.h>
#include <utility>
#include <stdexcept>
#include <boost/hana/assert.hpp>
#include <boost/hana/config.hpp>
#include <boost/hana/equal.hpp>
#include <boost/hana/fold_right.hpp>
#include <boost/hana/if.hpp>
#include <boost/hana/less.hpp>
#include <boost/hana/prepend.hpp>
#include <boost/static_assert.hpp>
#include <boost/hana/tuple.hpp>
#include <boost/hana/transform.hpp>
#include <boost/hana/detail/index_if.hpp>
#include <boost/hana/assert.hpp>
#include <boost/hana/find.hpp>
#include <boost/hana/back.hpp>
#include <boost/hana/integral_constant.hpp>
#include <boost/hana/at.hpp>
#include <boost/hana/fwd/front.hpp>
#include <boost/hana/optional.hpp>
#include <boost/hana/pair.hpp>
#include <boost/hana/tuple.hpp>
#include <boost/hana/type.hpp>
#include <boost/hana/unpack.hpp>
#include <boost/hana/size.hpp>
#include <boost/hana/for_each.hpp>
#include <boost/type_index.hpp>




namespace hana = boost::hana;
using namespace hana::literals;


namespace TypestateLibrary {

    struct func_info_t {
        const char* name;
    };

    template<auto state, auto FP, auto NextState>
    struct State {

        constexpr auto GetvalueState() {
            constexpr auto valueState = state;
            return valueState;

        }


        constexpr auto GetvalueFP() {
            constexpr auto valueFP = FP;
            return valueFP;
        }


        constexpr auto GetvalueNextState() {
            constexpr auto valueNextState = NextState;
            return valueNextState;

        }
        template <typename T>
        std::string  printState(T s) {
            std::string StateString = s._to_string();
           return StateString;
        }

    };



    template <typename... State>
    struct Typestate_Template {

        hana::tuple<State...> states;


        auto Get_start_state() {
            auto first_element = boost::hana::front(states);
        
        
            return first_element.GetvalueState();

        }
        auto Get_last_state() {
            auto last_element = boost::hana::back(states);
            return last_element;
        }

        template <typename T >
        void check_values(T state) {
            constexpr auto b = state.GetvalueFP();
         
        }

        template <typename T>
        void print_values(T state) {
            std::cout << "state= " << state.GetvalueState()
                << ", FunctionPointer= " << state.GetvalueFP()
                << ", NextState= " << state.GetvalueNextState() << "\n";

        }

        template<int x>
        auto GetAState() {
            return std::get<x>(states);
        }

        template <std::size_t... Is>
        auto iterate_states(std::index_sequence<Is...>) {

            (print_values(std::get<Is>(states)), ...);

        }

        void print_all_values() {

            hana::for_each(states, [&](auto x) {
                std::cout << "state= " << x.GetvalueState()
                    << ", FunctionPointer= " << x.GetvalueFP()
                    << ", NextState= " << x.GetvalueNextState() << "\n";
                    
                });
        }


        auto Get_target_state(auto b ,auto f) {

            auto first_element = boost::hana::front(states);
            auto result = hana::make_tuple();
            auto y = first_element.GetvalueState();
            auto z = first_element.GetvalueFP();
            auto w = first_element.GetvalueNextState();

            hana::for_each(states, [&](auto x) {
                
                if (b == x.GetvalueState() && reinterpret_cast<decltype(x.GetvalueFP())>(f) == x.GetvalueFP() ) {
                   
                       std::cout << x.GetvalueState() << std::endl;
                        y = x.GetvalueState();
                        z = reinterpret_cast<decltype(z)>(x.GetvalueFP());
                        w = x.GetvalueNextState();

     
                }
                });

            return hana::make_tuple(y, z, w);
        }


        auto find_any_time_method(auto m) {
            bool found = false;
            hana::for_each(states, [&](auto x) {
                if (reinterpret_cast<decltype(x.GetvalueFP())>(m) == x.GetvalueFP()) {
                    found = true;
                }
                });
            if (!found) {
                auto any_time_method = m;

                return any_time_method;
            }

        }


        template<typename T>
        void Visualize_TypestateTemplate() {

            
            hana::for_each(states, [&](auto state) {
                auto x = state.GetvalueState();
                
                });

            std::ofstream file("state_machine.dot");
            file << "digraph G {" << std::endl;

            hana::for_each(states, [&](auto state) {
                auto x = state.GetvalueState();
                file << "    " << static_cast<int>(x) << " [label=\"" << state.printState<T>(x) << "\"];" << std::endl;
            
                });
            auto last = Get_last_state();
           file << "    " << static_cast<int>(last.GetvalueNextState()) << " [label=\"" << last.printState<T>(last.GetvalueNextState()) << "\"];" << std::endl;


         
            hana::for_each(states, [&](auto state) {
           
            auto event = state.GetvalueFP();
            std::string func_type_name = typeid(event).name();
            std::string func_name = func_type_name.substr(func_type_name.find("(__cdecl") + 9);

            
           //func_name = func_name.substr(func_name.find("__ptr64)")+4);
            auto fromState = state.GetvalueState();
            auto toState =  state.GetvalueNextState();
            file << "    " << static_cast<int>(fromState) << " -> " << static_cast<int>(toState)
                << " [label=\"" << func_name << "\"];" <<
                std::endl;    
                
                });

            // Close the Graphviz file
            file << "}" << std::endl;
            file.close();
            // Convert the filename to a wide-character string
            int len = strlen("Typestates.png") + 1;
            WCHAR* wstr = new WCHAR[len];
            MultiByteToWideChar(CP_UTF8, 0, "Typestates.png", -1, wstr, len);

            // Use the Graphviz command-line tool to convert the .dot file to an image
            system("dot -Tpng state_machine.dot -o Typestates.png");

            // Open the image in the default image viewer
            ShellExecute(NULL, L"open", wstr, NULL, NULL, SW_SHOWNORMAL);

            // Free the wide-character string
            delete[] wstr;


        }


    };
    
    
    
    

    

   
    template <typename T, typename  Typestate_Template>
    class Tracked : public T {
    protected:
        // static instances map that holds pointers to all instances of classes 
        //that inherit from Tracked class
        
        Tracked() {
            counter_++;
            TrackedInstances_.insert(std::make_pair(reinterpret_cast<T*>(this), V));
           // std::cout << "Creating instance " << this << " at state " << V << std::endl;

        }

        ~Tracked() {
            TrackedInstances_.erase(reinterpret_cast<T*>(this));
           // std::cout << "Destroying instance " << this << " at state " << V << std::endl;
        }
    protected:
        template <typename... Args>
        auto Typestate_Checking(void (T::* func)(Args...), Args... args) -> decltype((std::declval<T>().*func)(args...)) {
            std::cout << "Calling method " << typeid(func).name() << " on instance " << this << std::endl;
            return (static_cast<T*>(this)->*func)(args...);
        }

        static std::map<T*, int>& TrackedInstances() {

            return TrackedInstances_;
        }

    private:

        static int counter_;
        static std::map<T*, int> TrackedInstances_;

        Typestate_Template Prot;
        int V = Prot.Get_start_state();

    };

    template<typename T, typename  Typestate_Template>
    int Tracked<T, Typestate_Template>::counter_ = 0;

    template<typename T, typename  Typestate_Template>
    std::map< T*, int> Tracked<T, Typestate_Template>::TrackedInstances_;

   

    // The is_tracked trait is used to determine whether a class inherits from Tracked.
    template <typename T, typename  Typestate_Template>
    struct is_tracked : std::integral_constant<bool, std::is_base_of<Tracked<T, Typestate_Template>, T>::value> {};

    template <typename T, typename P, typename = void>
    class TypestateClassConnector : public T {};

   
    template <typename T, typename  Typestate_Template>
    class TypestateClassConnector<T, Typestate_Template> : public T, public Tracked<T, Typestate_Template> {




    private:
        using T::T;
        T obj;
        Typestate_Template Prot;
        int size = sizeof(Prot.states);
        static int counter_1;


      

        template <typename F, typename... Args>
       
        void Typestate_Checking(F func, Args... args) {
            
            const auto AnyMethod = Prot.find_any_time_method(func);
            if (reinterpret_cast<decltype(AnyMethod)>(func) == AnyMethod) {

               // std::cout << "Calling anytime method " << typeid(func).name() << " on instance " << this << std::endl;

            }
            
            else {

                static std::map<T*, int>& mapRef = this->TrackedInstances();
                // std::cout << " object " << this << " at state " << mapRef[this] << std::endl;
                auto cuerrent_state = Prot.Get_target_state(mapRef[this], func);
                auto defstate = boost::hana::front(Prot.states);

                const auto cs = boost::hana::at_c<0>(cuerrent_state);
                const auto FP = boost::hana::at_c<1>(cuerrent_state);
                const  auto NS = boost::hana::at_c<2>(cuerrent_state);
                const auto value = reinterpret_cast<decltype(FP)>(func);
                const auto cuerr = mapRef[this];

                
                
                if (cs == mapRef[this] && reinterpret_cast<decltype(FP)>(func) == FP)

                {
                    
                    //// std::cout << mapRef[this] << " = " << cs << " and "
                          //  << func << " = " << FP << std::endl;
                        mapRef[this] = NS;

                       // std::cout << "Calling method " << typeid(func).name() << " on instance " << this << std::endl;
                       //std::cout << "Now :" << this << " at state " << mapRef[this] << std::endl;
                   
                }

                else {
                  
                    static_assert(std::is_same<decltype(FP), decltype(value)>::value,
                        "Calling method Cause errors, Follow the typestate specfications!");
                   //if (FP != value) {
                    //    const bool found = false;
                      //  CheckFoo<false>{};
                  //  }
              //   assert(FP==value);
              
                }
            
                

            }

        }


        
      
     
    public:
        template<typename... Args>
        constexpr auto operator->*(auto (T::* funcPtr)(Args...))  {

            return [this, funcPtr](Args&&... args) -> decltype(auto)  {
                
                Typestate_Checking(funcPtr, std::forward<Args>(args)...);
                return  (this->*funcPtr)(std::forward<Args>(args)...);
            };
        }


      




    };


    //////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////
             /**************Subtyping Magic goes here ********************/





    template <typename Typestate_template_1, typename Typestate_template_2>
    struct Merged_Typestate {
       
       


        auto Get_start_state_Combined() {
            auto concatenated = hana::concat(m1.states, m2.states);
            auto first_element = boost::hana::front(concatenated);
            //  std::cout << "Start state for the object is " << first_element.GetvalueState();
                //return std::get<0>(states).GetvalueState();
            return first_element.GetvalueState();

        }

        auto Get_from_combine_both(auto b, auto f) {

            auto concatenated = hana::concat(m1.states, m2.states);

            auto first_element = boost::hana::front(concatenated);
            auto result = hana::make_tuple();
            auto y = first_element.GetvalueState();
            auto z = first_element.GetvalueFP();
            auto w = first_element.GetvalueNextState();

            hana::for_each(concatenated, [&](auto state) {
                if (b == static_cast<decltype(b)>(state.GetvalueState()) && reinterpret_cast<decltype(state.GetvalueFP())>(f) == state.GetvalueFP()) {

                   std::cout << state.GetvalueState() << " : "<< state.GetvalueFP()<< ":"<<state.GetvalueNextState()<<std::endl;
                    y = static_cast<decltype(y)>(state.GetvalueState());
                    z = reinterpret_cast<decltype(z)>(state.GetvalueFP());
                    w = static_cast<decltype(w)>(state.GetvalueNextState());


                }


              
                });
            return hana::make_tuple(y, z, w);

        }

    private:
        Typestate_template_1 m1;
        Typestate_template_2 m2;

       


    };


    template <typename T, typename  Merged_Typestate>
    class TrackedForSubtyping : public T {
    protected:
        // static instances map that holds pointers to all instances of classes 
       
        TrackedForSubtyping() {
            counter_++;
            TrackedInstancesSubTyping_.insert(std::make_pair(reinterpret_cast<T*>(this), V));
         //   std::cout << "Creating instance " << this << " at state " << V << std::endl;

        }

        ~TrackedForSubtyping() {
            TrackedInstancesSubTyping_.erase(reinterpret_cast<T*>(this));
            // std::cout << "Destroying instance " << this << " at state " << V << std::endl;
        }
    protected:
        template <typename... Args>
        auto Typestate_Checking(void (T::* func)(Args...), Args... args) -> decltype((std::declval<T>().*func)(args...)) {
            std::cout << "Calling method " << typeid(func).name() << " on instance " << this << std::endl;
            return (static_cast<T*>(this)->*func)(args...);
        }

        static std::map<T*, int>& TrackedInstances() {

            return TrackedInstancesSubTyping_;
        }

    private:

        static int counter_;
        static std::map<T*, int> TrackedInstancesSubTyping_;

        Merged_Typestate Prot;
        int V = Prot.Get_start_state_Combined();

    };

    template<typename T, typename  Merged_Typestate>
    int TrackedForSubtyping<T, Merged_Typestate>::counter_ = 0;

    template<typename T, typename  Merged_Typestate>
    std::map< T*, int> TrackedForSubtyping<T, Merged_Typestate>::TrackedInstancesSubTyping_;



    // The is_tracked trait is used to determine whether a class inherits from Tracked.
    template <typename T, typename  Merged_Typestate>
    struct is_tracked_Subtyping : std::integral_constant<bool, std::is_base_of<TrackedForSubtyping<T, Merged_Typestate>, T>::value> {};

    template <typename T, typename P, typename = void>
    class Subtyping_Checker : public T {};

    template <typename T, typename   Merged_Typestate>
    class Subtyping_Checker<T, Merged_Typestate> : public T, public TrackedForSubtyping<T, Merged_Typestate> {


    private:
        using T::T;
        T obj;
        Merged_Typestate Prot;
       
        static int counter_1;

        template <typename F, typename... Args>

        void Typestate_Checking(F func, Args... args) {

           

                static std::map<T*, int>& mapRef = this->TrackedInstances();
                std::cout << " object " << this << " at state " << mapRef[this] << std::endl;
                auto cuerrent_state = Prot.Get_from_combine_both(mapRef[this], func);
              
                const auto cs = boost::hana::at_c<0>(cuerrent_state);
                const  auto FP = boost::hana::at_c<1>(cuerrent_state);
                const  auto NS = boost::hana::at_c<2>(cuerrent_state);
                const auto value = reinterpret_cast<decltype(FP)>(func);
                const auto cuerr = mapRef[this];


                const bool found = false;
                if (cs == mapRef[this] && reinterpret_cast<decltype(FP)>(func) == FP)

                {

                  // std::cout << mapRef[this] << " = " << cs << " and "
                      //<< func << " = " << FP << std::endl;
                   mapRef[this] = NS;
                   // std::cout << "Calling method " << typeid(func).name() << " on instance " << this << std::endl;
                   // std::cout << "Now :" << this << " at state " << mapRef[this] << std::endl;

                }

                else {
                static_assert(std::is_same<decltype(FP), decltype(value)>::value,
                        "Calling method Cause errors, Follow the typestate specfications!");
                //    assert(FP == value);
                    // static_assert(std::is_same_v<decltype(func), decltype(FP)>, "Not the Same!" ) ;
                   // static_assert(false,"Wrong tranision");
                }
         




        }


    public:
        template<typename... Args>
        constexpr auto operator->*(auto (T::* funcPtr)(Args...)) {

            return [this, funcPtr](Args&&... args) -> decltype(auto) {

                Typestate_Checking(funcPtr, std::forward<Args>(args)...);
                return  (this->*funcPtr)(std::forward<Args>(args)...);
            };
        }

    };


}
