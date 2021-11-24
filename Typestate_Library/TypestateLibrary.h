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

using namespace std;
//using namespace std;




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
 template < auto CST, auto FunctionPointer, typename... map_protocol>
struct find_transition{
  using type = not_There;
};
 //Found the a normal transition.
template <auto NST, auto CST, auto FunctionPointer,typename... map_protocol>
struct find_transition<CST, FunctionPointer,map_transition<CST, NST, FunctionPointer>,
map_protocol...> {
  using type = map_transition<CST, NST,FunctionPointer>;
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






//analaysing the protocl
template <typename T>
struct unwrap_of_function {
  static_assert(!std::is_same_v<T, not_There>, "Transition is not defind");
};

template <auto CST, auto FunctionPointer, typename... map_transition>
struct find_transition<CST, FunctionPointer,
                         map_protocol<map_transition...> > {
  using type = typename find_transition<CST, FunctionPointer,
                                          map_transition...>::type;
};


//wrap it to find the function for each transition  Needs the arguments for result_of

template <auto CST, auto NST, auto FunctionPointer >
struct unwrap_of_function<map_transition< CST, NST, FunctionPointer>> {
  static constexpr auto EndState = NST;
};

template <typename map_protocol, auto CST, auto FunctionPointer>
constexpr auto return_of_transition =
unwrap_of_function<
find_transition<CST, FunctionPointer, map_protocol>>::EndState;



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



template<class T>
struct is_pointer_function : std::false_type {};

template<class R, class T, class... Args>
struct is_pointer_function<R (T::*)(Args...) &&>
: std::true_type {};








template <auto CST, template <auto State> typename ImplementaClass,typename CPS, typename map_protocol>

class Checker{
public:
    template <auto NewState>
    using ThisClass = Checker<NewState, ImplementaClass, CPS, map_protocol>;

    // Check that the transition is valid, then call the function, and return the
    // wrapper with the updated state.
    template <auto FunctionPointer, typename... Args>
      auto call_transition(Args&&... args) && {
        (CPS_.*FunctionPointer)(forward<Args>(args)...);
        constexpr auto target_state =
          return_of_transition<map_protocol, CST, FunctionPointer>;
        return ImplementaClass<target_state>(
                  ThisClass<target_state>{move(CPS_), true});
      }
    
private:
    CPS CPS_;
};





}


#define Class_Contain(ImplementaClass, CPS,            \
map_protocol, State_type) {   \                               \
  template<State_type  CST>                                            \
  class Implemented                                                         \
    : public :: TypestateTool::   Checker<                      \
          CST, , ImplementaClass, CPS,            \
    map_protocol>                       \
  {                                                                            \
   public:                                                                     \
    using ID =                                                               \
:: TypestateTool::   Checker<                                  \
CST, , ImplementaClass, CPS,            \
map_protocol>;                    \
    using Prevouis = CPS;                                              \
   private:                                                                    \
                                                    \
    template<auto, template <auto> typename, typename, typename, typename,     \
             typename, typename>                                               \
    friend class :: TypestateTool::   Checker;                         \
ImplementaClass(ID&& other) : ID(other) {} \

#define Class_Contain_END } }







