

//
//  TypestateLibrary.h
//  Typestate_C++
//
//  Created by Arwa Alsubhi on 22/11/2021.
//

#ifndef TypestateLibrary_h
#define TypestateLibrary_h

/*
 
 it is a Typestate Tool which makes the user able to write a program in C++ with typestate specification
 attached to it, then compile it, and run it as a normal C++ program.
 Therefore, a typestate tool will significantly facilitate and secure the
 user’s work by detecting any violations of the protocol that the user
 specifies for objects.
 */
#endif /* TypestateLibrary_h */

#include <utility>
#include <cstdlib>

#include <assert.h>
#include <string>

#include<iostream>

using namespace std;
//using namespace std;




namespace TypestateTool{
  
 
 // to be used by users to store one transition form state to state by using a pointer function in Map style format
 template <auto CST , auto NST, auto Pointer >
 struct map_transition;

 
//another way of writing protocol,  swhich templates to store one transition at a time
template<class CST, class NST, auto Pointer, class State>
void Swhich_Protocol(CST a , NST b, State s) {
    switch(s){
    case a:
    Pointer->b;
            break;
    }}



//another way of writing protocol,if-else statements to be used by users to store one transition at time
template<class CST, class NST, auto Pointer,class State>

void IF_ELSE_Protocol(CST a, NST b,State s)
{
    if(s==a){
        Pointer->b;
    }
}
// another way of writing protocol, by using class templates 
//class templates for states
template<class state>
struct className;
//templates for transition
template<auto pointer>
struct funcPointer;
//to be used by users to store transtioin
template<typename className ,typename funcPointer>
struct single_transition;





 // this container is used to store all transitions for map transition templates
 template <typename...map_transition>
 struct map_protocol;

// after user define the protocol,  library should find transitions that applied by users
// take size of map_protocol arguments (number of transitions that user enter )
template<typename...map_protocol>
struct take_num_of_transitions{
  const std::size_t n = sizeof...(map_protocol);
};
 
 
 
 // track the states of any defined protocol

template <typename T, typename... Args>
struct unwrap_of_states {
  static_assert(!std::is_same_v<T, decltype(false)>, "Next State is not defind");
static_assert(!std::is_same_v<T, decltype(false)>, "Current State is not defind");
};
// analyzes them and save them
template <auto CST, auto NST, auto Pointer, typename... Args>
struct unwrap_of_states<map_transition<CST,NST, Pointer>,
                               Args...> {
  using type = std::result_of_t<decltype(NST)(Args...)>;
using type2 = std::result_of_t<decltype(NST)(Args...)>;
};

 // try to find the protocol
 
 // this function would return error when the library dose not find the a certian transition 
 template < auto CST, auto NST, auto Pointer, typename... map_protocol>
struct Search_transition{
  using type = decltype(false);
};

 // this template will return the transition when they find it 
template <auto NST, auto CST, auto Pointer,typename take_num_of_transitions,typename... map_protocol>
struct Search_transition<CST, NST,Pointer,take_num_of_transitions,map_transition<CST, NST, Pointer>,
take_num_of_transition,map_protocol...> {

        using type = map_transition<CST, NST,Pointer>;
};


// after Found the  transition.  .
//analaysing of the protocl should be done
template <typename T>
struct unwrap_of_function {
  static_assert(!std::is_same_v<T, decltype(false)>, "You are in Wrong Transition use another function");
};

template <auto CST, auto NST, auto Pointer, typename take_num_of_transitions,typename... map_transition>
struct Search_transition<CST, NST,Pointer, take_num_of_transitions,
                         map_protocol<map_transition...> > {
  using type = typename Search_transition<CST,NST, Pointer,
                                          map_transition...>::type;
};
 



//extract the pointer to the function for each transition

template <auto CST, auto NST, auto Pointer >
struct unwrap_of_function<map_transition< CST, NST, Pointer>> {
    static auto CurrentState = CST;
  static  auto NextState = NST;
};
 
// track pointer in order to check the validiy of the transition.
 
// check at what state the pointer is
template <typename map_protocol, auto CST, auto NST, auto Pointer>
static constexpr auto vaild_transition_Start =
unwrap_of_function<Search_transition<CST, NST,Pointer, map_protocol>>:: CurrentState;
// check at where it is going 
template <typename map_protocol, auto CST, auto NST, auto Pointer>
static constexpr  auto vaild_transition_END =
unwrap_of_function<Search_transition<CST, NST, Pointer, map_protocol>>:: NextState;

 



// this checker will use templates above to check protocol validation


template <typename Class, typename map_protocol>
class Checker{
public:
    Class State;
    Class classID;
    using ThisClass = Checker<Class, map_protocol>;
    // Check that the transition is valid, and then move the object pointer to new right state. if is not valid return error 
    template <auto Pointer, typename... Args>
      auto Check_transition(Args&&... args) && {
        (classID*Pointer)(forward<Args>(args)...);
          constexpr auto prevois_state =vaild_transition_Start<map_protocol, State, Pointer>;
        constexpr auto right_state =
          vaild_transition_END<map_protocol, State, Pointer>;
          
          return Pointer->right_state;
      }
    
    
    

    
   
};




    
    
    
    
}








// this Maroc function to pass the class and the protocol to the Checker class

#define Assign_to_Class(Class, map_protocol) \
Class StartState; \
  class Check                                                         \
    : public ::TypestateTool::Checker<                             \
 Class,            \
map_protocol>                                          \
  {                                                                            \
   public:                                                                     \
    using ID =                                                               \
::TypestateTool::Checker<                                  \
  Class,         \
map_protocol>;                    \
                                                \
   \
    template<typename, typename  \
             >                                             \
    friend class ::TypestateTool::Checker;   \
                                                            \
\
       \
                  \
}\






