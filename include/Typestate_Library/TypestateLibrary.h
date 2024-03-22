#pragma once
#include <iostream>
#include "enum.h" 
#include <functional>
#include <map>
#include <assert.h>
#include <stdexcept>
#include <fstream>
#include <sstream>
#include "enum.h"
#include <functional>
#include <map>
#include <assert.h>
#include <utility>
#include <stdexcept>
#include <boost/hana/equal.hpp>
#include <boost/hana/fold_right.hpp>
#include <boost/hana/if.hpp>
#include <boost/hana/less.hpp>
#include <boost/hana/fold.hpp>
#include <boost/hana/tuple.hpp>
#include <boost/hana/transform.hpp>
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


#define ANSI_COLOR_RED "\x1b[31m"
#define ANSI_COLOR_RESET "\x1b[0m"




// Handles errors by printing a message and exiting the program.
#define HANDLE_ERROR(msg) do { \
    std::cerr << ANSI_COLOR_RED << "Error: " << msg << ANSI_COLOR_RESET << std::endl; \
    std::exit(EXIT_FAILURE); \
} while (0)

namespace TypestateLibrary {

    /*
    Struct `State` represents a single state within the typestate pattern. It encapsulates
    a state, a function pointer as an action, and the next possible state. This struct
    is crucial for peresent each valid transition made between
    valid states as per the typestate specification.
    */

    template<auto state, auto FP, auto NextState>
    struct State {
        using StateType = decltype(state);
        using FPType = decltype(FP);
        using NextStateType = decltype(NextState);

        // Validates the function pointer against the stored function pointer.
        template<auto f>
        constexpr bool check_value() {
            return reinterpret_cast<decltype(FP)>(f) == FP;
        }

        // Retrieves the current state.
        constexpr auto GetValueState() {
            return state;
        }

        // Retrieves the function pointer associated with the current state.
        constexpr auto GetValueFP() {
            return FP;
        }

        // Retrieves the next state following the current one.
        constexpr auto GetValueNextState() {
            return NextState;
        }

        // Converts state to string representation. 
        template <typename T>
        std::string GetStateName(T s) {
            return s._to_string();
        }

        // Prints the current state to a string.
        template <typename T>
        std::string PrintState(T s) {
            return s._to_string();
        }
    };


    /*
    Struct `Typestate_Template` manages a collection of `State` instances, defining
    the entire sequence of states and transitions for a typestate pattern. It supports
    compile-time checks to ensure correct transitions and provides utilities for
    accessing state information and visualizing the state machine.
    */

    template <typename... States>
    struct Typestate_Template {
        hana::tuple<States...> states;

        // Retrieves the starting state of the typestate sequence.
        auto GetStartState() {
            return hana::front(states).GetValueState();
        }

        // Retrieves the last state in the typestate sequence.
        auto GetLastState() {
            return hana::back(states);
        }

        // Prints values associated with a given state, aiding in debugging and visualization.
        template <typename T>
        void PrintValues(T state) {
            std::cout << "State= " << state.GetValueState()
                      << ", FunctionPointer= " << state.GetValueFP()
                      << ", NextState= " << state.GetValueNextState() << std::endl;
        }

        // Iterates through and prints all states and their associated values.
        void PrintAllValues() {
            hana::for_each(states, [&](auto x) {
                PrintValues(x);
            });
        }

        // Determines the target state based on a given state and function pointer.
        auto Get_target_state(auto b, auto f) {
            auto first_element = boost::hana::front(states);
            auto result = hana::make_tuple();
            auto y = first_element.GetValueState();
            auto z = first_element.GetValueFP();
            auto w = first_element.GetValueNextState();

            hana::for_each(states, [&](auto x) {
                if (b == x.GetValueState() && reinterpret_cast<decltype(x.GetValueFP())>(f) == x.GetValueFP()) {
                    
                    y = x.GetValueState();
                    z = reinterpret_cast<decltype(z)>(x.GetValueFP());
                    w = x.GetValueNextState();
                }
            });

            return hana::make_tuple(y, z, w);
        }

        // Identifies if a method can be called at any time within the typestate sequence.
        auto find_any_time_method(auto m) {
            bool found = false;
            hana::for_each(states, [&](auto x) {
                if (reinterpret_cast<decltype(x.GetValueFP())>(m) == x.GetValueFP()) {
                    found = true;
                }
            });
            if (!found) {
                auto any_time_method = m;
                return any_time_method;
            }
        }

        // Generates a graph representation of the state transitions for visualization.
        void Visualize_TypestateTemplate() {
            std::ofstream file("state_machine.dot");
            file << "digraph G {" << std::endl;

            hana::for_each(states, [&](auto state) {
                auto x = state.GetValueState();
                file << "    " << static_cast<int>(x) << " [label=\"" << state.PrintState(x) << "\"];" << std::endl;
            });

            auto last = GetLastState();
            file << "    " << static_cast<int>(last.GetValueNextState()) << " [label=\"" << last.PrintState(last.GetValueNextState()) << "\"];" << std::endl;

            hana::for_each(states, [&](auto state) {
                auto fromState = state.GetValueState();
                auto toState = state.GetValueNextState();
                file << "    " << static_cast<int>(fromState) << " -> " << static_cast<int>(toState)
                     << " [label=\"Transition\"];" << std::endl; // Simplified to "Transition"
            });

            file << "}" << std::endl;
            file.close();
        }

    };

   // Primary template for function_traits, left undefined on purpose.
   template<typename T>
    struct function_traits;

  // Specialization for non-const member function pointers
    template<typename C, typename R, typename... Args>
     struct function_traits<R(C::*)(Args...)> {
      using return_type = R;
      };

   // Specialization for const member function pointers
     template<typename C, typename R, typename... Args>
     struct function_traits<R(C::*)(Args...) const> {
      using return_type = R;
      };

    /*
    Class `State_Manager` enforces the typestate pattern while leveraging
    compile-time defined states and transitions. It tracks instances of objects, ensuring
    they move through their defined states according to the typestate specification.
    */
 


    template <typename T, typename TypestateTemplate>
    class State_Manager : public T {
    protected:
        static int counter_;
        static std::map<T*, int> TrackedInstances_;
        TypestateTemplate Prot;
        int V = Prot.GetStartState();

    public:
        State_Manager() {
            counter_++;
            TrackedInstances_.insert(std::make_pair(reinterpret_cast<T*>(this), V));
            //std::cout << "Creating instance " << this << " at state " << V << std::endl;
        }

        ~State_Manager() {
            TrackedInstances_.erase(reinterpret_cast<T*>(this));
            //std::cout << "Destroying instance " << this << " at state " << V << std::endl;
        }

        // Static method to access all tracked instances.
        static std::map<T*, int>& GetTrackedInstances() {
            return TrackedInstances_;
        }

        // Performs typestate checking before invoking a method on an instance.
        template <typename F, typename... Args>
        void Typestate_Checking(F func, Args... args) {
            const auto AnyMethod = Prot.find_any_time_method(func);

            if (reinterpret_cast<decltype(AnyMethod)>(func) == AnyMethod) {
                //std::cout << "Calling anytime method " << typeid(func).name() << " on instance " << this << std::endl;
            } else {
                static std::map<T*, int>& mapRef = this->GetTrackedInstances();
                const auto current_state = Prot.Get_target_state(mapRef[this], func);
                const auto cs = boost::hana::at_c<0>(current_state);
                const auto FP = boost::hana::at_c<1>(current_state);
                const auto NS = boost::hana::at_c<2>(current_state);

                if (cs == mapRef[this] && reinterpret_cast<decltype(FP)>(func) == FP) {
                    mapRef[this] = NS;
                } else {

                    // Handle error for invalid state transition or function call.
                    using func_type = decltype(func);
                    using return_type = function_traits<func_type>::return_type;
                    using fp_type = decltype(FP);
                    using return_type_FP = function_traits<func_type>::return_type;
    
                    static_assert(std::is_same<return_type, return_type_FP>::value,
                    "Calling method causes errors, follow the type state specifications!");
                   
                    std::cerr << "Error: Invalid state transition or function call." << std::endl;
                    HANDLE_ERROR("State transition or function call does not match the allowed transitions.");
                }
            }
        }
    };


    


    template<typename T, typename TypestateTemplate>
    int State_Manager<T, TypestateTemplate>::counter_ = 0;

    template<typename T, typename TypestateTemplate>
    std::map<T*, int> State_Manager<T, TypestateTemplate>::TrackedInstances_;

    /*
    Struct `is_tracked` is a compile-time check to determine if a given type is managed
    by a `State_Manager`, ensuring it adheres to a typestate pattern. This facilitates
    compile-time validation of the object lifecycle according to the typestate.
    */

    template <typename T, typename TypestateTemplate>
    struct is_tracked : std::integral_constant<bool, std::is_base_of<State_Manager<T, TypestateTemplate>, T>::value> {};

    /*
    Class `TypestateClassConnector` integrates typestate functionality with user-defined
    types. It intercepts method calls to perform typestate checks, ensuring that object
    interactions adhere to the defined typestate pattern. 
    */

    template <typename T, typename TypestateTemplate>
    class TypestateClassConnector : public State_Manager<T, TypestateTemplate> {
    public: 
       
        template<typename... Args>
        auto operator->*(auto (T::* funcPtr)(Args...)) {
            return [this, funcPtr](Args&&... args) -> decltype(auto) {
                this->Typestate_Checking(funcPtr, std::forward<Args>(args)...);
                return (this->*funcPtr)(std::forward<Args>(args)...);
            };
        }
    };


}
