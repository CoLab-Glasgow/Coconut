//
//  TypestateLibrary.h
//  Typestate_C++
//
//  Created by Arwa Alsubhi on 22/11/2021.
//

#ifndef TypestateLibrary_h
#define TypestateLibrary_h


#endif /* TypestateLibrary_h */

#include <utility>
#include <cstdlib>
#include <tuple>
#include <variant>
#include <assert.h>
#include <string>

#include<iostream>


//using namespace std;

#define Typestate_MACRO_END Typestate_MACRO_END_1(__LINE__)

namespace TypestateTool{
  
 
 // to be used by users to store map transtioin form state to state for non repeated transtiopns
 template <auto CST , auto NST, auto FunctionPointer >
 struct map_transition;


 
 //to be used by users to store map transtioin form state to state for non repeated transtiopns
template <auto CST , auto NST , auto FunctionPointer>
struct map_Repeated_transition;


 // a map to store all transitions
 template <typename...map_transition>
 struct map_protocol;
 
 // assign a protocl to class
template<class T, typename... map_protocol>
struct assign_to_class{};

// find transitions that applied by users
// take size of map_protocol arguments (number of transitions that user enter )
template<typename...map_protocol>
struct take_num_of_transition{
  const std::size_t n = sizeof...(map_protocol);
//return n;
};




 struct not_There{};
 //not found transition
 template <auto NST, auto CST, auto FunctionPointer, typename... map_protocol>
struct find_transition{
  using type = not_There;
};
 //Found the a normal transition.
 template <auto NST, auto CST, auto FunctionPointer, typename... map_protocol>
struct find_transition<CST, NST, FunctionPointer, map_transition<CST, NST, FunctionPointer>, map_protocol...> {
  using type = map_transition<CST, NST, FunctionPointer>;
};

template <auto NST, auto CST, auto FunctionPointer, typename... map_protocol>
struct find_transition_Repeated{
  using type = not_There;
};


// Found the a repeated transition.
template <auto NST, auto CST, auto FunctionPointer, typename... map_protocol>
struct find_transition_Repeated<CST, NST, FunctionPointer, map_Repeated_transition<CST, NST, FunctionPointer>, map_protocol...> {
  using type = map_Repeated_transition<CST, NST, FunctionPointer>;
};
//




// after Found the  transition.  .
template <auto NST, auto CST, auto FunctionPointer, typename... map_transition>
struct find_transition<CST, NST, FunctionPointer, map_protocol<map_transition...> > {
  using type = typename find_transition<CST, NST, FunctionPointer,
                                          map_transition...>::type;
  
};

//analysing the protocl
  
  
  // fine defined class for the protocol 
  
template<typename T, typename...Args>
struct unwrap_Class{
    static_assert(!std::is_same_v<T, not_There>, "Class is not defind");
  
};
  
template<class T, typename... map_transition, typename... Args>
struct unwrap_Class<assign_to_class<T,map_protocol<map_transition...>, Args...>>{
    
    using type = std::result_of_t< decltype(T)(Args...)>;
    
};

  
  
template <typename T, typename... Args>
struct unwrap_of_transition {
  static_assert(!std::is_same_v<T, not_There>, "Transition is not defind");
};

//wrap it to find the function for each transition  Needs the arguments for result_of
template <auto CST, auto NST, auto FunctionPointer, typename... Args>
struct unwrap_of_transition<map_transition<CST,NST, FunctionPointer>,
                               Args...> {
  using type = std::result_of_t<decltype(FunctionPointer)(Args...)>;
};



template <typename T, typename... Args>
struct unwrap_of_CurrentState {
  static_assert(!std::is_same_v<T, not_There>, "Current State is not defind");
};

template <auto CST, auto NST, auto FunctionPointer, typename... Args>
struct unwrap_of_CurrentState<map_transition<CST,NST, FunctionPointer>,
                               Args...> {
  using type = std::result_of_t<decltype(CST)(Args...)>;
};

template <typename T, typename... Args>
struct unwrap_of_NextState {
  static_assert(!std::is_same_v<T, not_There>, "Next State is not defind");
};

template <auto CST, auto NST, auto FunctionPointer, typename... Args>
struct unwrap_of_NextState<map_transition<CST,NST, FunctionPointer>,
                               Args...> {
  using type = std::result_of_t<decltype(NST)(Args...)>;
};









}


