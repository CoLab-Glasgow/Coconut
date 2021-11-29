//
//  TypestateLibrary.h
//  Typestate_C++
//
//  Created by Arwa Alsubhi on 22/11/2021.
//

#ifndef TypestateLibrary_h
#define TypestateLibrary_h


#endif /* TypestateLibrary_h */
/*
 
 it is a Typestate Tool which makes the user able to write a program in C++ with typestate specification
 attached to it, then compile it, and run it as a normal C++ program.
 Therefore, a typestate tool will significantly facilitate and secure the
 user’s work by detecting any violations of the protocol that the user
 specifies for objects.

 */
#include <utility>
#include <cstdlib>

#include <assert.h>
#include <string>

#include<iostream>

using namespace std;




namespace TypestateTool{
  
 
 // to be used by users to store map transtioin form state to state for non repeated transtiopns
 template <auto CST , auto NST, auto Pointer >
 struct map_transition;


 
 

 // a map to store all transitions
 template <typename...map_transition>
 struct map_protocol;

// find all transitions that applied by users

template<typename...map_protocol>
struct take_num_of_transition{
    bool type = false;
    static_assert(type== false, "Transitions are not Defined");

};
// take size of map_protocol arguments (number of transitions that user enter )
 // when find transitions
template<typename...map_protocol>
struct take_num_of_transition{
  const std::size_t n = sizeof...(map_protocol);
};


// for each transition do the analysis
 
 //not found transition
 template < auto CST, auto Pointer, typename... map_protocol>
struct find_transition{
  bool type = false;
};
 //take the transition when find it.
template <auto NST, auto CST, auto Pointer,typename... map_protocol>
struct find_transition<CST, Pointer,map_transition<CST, NST, Pointer>,
map_protocol...> {
  using type = map_transition<CST, NST,Pointer>;
};




// then analysis .

 
template <typename T, typename... Args>
struct unwrap_of_CurrentState {
  static_assert(T==false, "Current State is not defind");
};

template <auto CST, auto NST, auto Pointer, typename... Args>
struct unwrap_of_CurrentState<map_transition<CST,NST, Pointer>,
                               Args...> {
  using type = std::result_of_t<std::declval(CST)(Args...)>;
};

template <typename T, typename... Args>
struct unwrap_of_NextState {
    static_assert(T==false, "Next State is not defind");
};

template <auto CST, auto NST, auto Pointer, typename... Args>
struct unwrap_of_NextState<map_transition<CST,NST, Pointer>,
                               Args...> {
  using type = std::result_of_t<std::declval(NST)(Args...)>;
};
template <typename T>
struct unwrap_of_function {
  static_assert(T==false, "You are in Wrong Transition use another function");
};

 
 
template <auto CST, auto Pointer, typename... map_transition>
struct find_transition<CST, Pointer,
                         map_protocol<map_transition...> > {
  using type = typename find_transition<CST, Pointer,
                                          map_transition...>::type;
                             return type;
};






template <auto CST,  template <auto State> typename Check, typename Class, typename map_protocol>
struct Checker{
    Class classID;
    template <auto NewState>
    using ThisClass = Checker<NewState, Class, map_protocol>;
    // Check that the transition is valid, and then return the object pointer to target state.
    
    template <auto Pointer, typename... Args>
      auto Check_transition(Args&&... args) && {
        (classID*Pointer)(forward<Args>(args)...);
    
        constexpr auto target_state =
          vaild_transition<map_protocol, CST, Pointer>;
        return ClassID<target_state>(
                  ThisClass<target_state>{move(classID), true});
      }
    

   
  
 
   
};







}



#define Assign_to_Class(Check, CLass, type, map_protocol) \
template<type CST>  \
  struct Check                                                         \
    : public ::TypestateTool::Checker<                             \
          CST, Check, CLass,            \
map_protocol>                                          \
  {                                                                            \
   public:                                                                     \
    using ID =                                                               \
::TypestateTool::Checker<                                  \
            CST, Check,  CLass,         \
map_protocol>;                    \
    using class_id = CLass;                                              \
  \
                                             \
            \
                                                             \
\
       \
                  \
}\




