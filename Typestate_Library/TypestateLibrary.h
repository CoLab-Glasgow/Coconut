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
 
 this is a Typestate library make the user able to write a program in C++ with typestate specification
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
//using namespace std;




namespace TypestateTool{
  
 
 // to be used by users to store map transtioin form state to state for non repeated transtiopns
 template <auto CST , auto NST, auto Pointer >
 struct map_transition;


 
 //to be used by users to store map transtioin form state to state for non repeated transtiopns
template <auto CST , auto NST , auto Pointer>
struct map_Repeated_transition;


 // a map to store all transitions
 template <typename...map_transition>
 struct map_protocol;


// find transitions that applied by users
// take size of map_protocol arguments (number of transitions that user enter )
template<typename...map_protocol>
struct take_num_of_transition{
  const std::size_t n = sizeof...(map_protocol);
//return n;
};



 struct not_There{};
 //not found transition
 template < auto CST, auto Pointer, typename... map_protocol>
struct find_transition{
  using type = not_There;
};
 //Found the a normal transition.
template <auto NST, auto CST, auto Pointer,typename... map_protocol>
struct find_transition<CST, Pointer,map_transition<CST, NST, Pointer>,
map_protocol...> {
  using type = map_transition<CST, NST,Pointer>;
};




// after Found the  transition.  .






//analaysing the protocl
template <typename T>
struct unwrap_of_function {
  static_assert(!std::is_same_v<T, not_There>, "You are in Wrong Transition use another function");
};

template <auto CST, auto Pointer, typename... map_transition>
struct find_transition<CST, Pointer,
                         map_protocol<map_transition...> > {
  using type = typename find_transition<CST, Pointer,
                                          map_transition...>::type;
};


//wrap it to find the function for each transition  Needs the arguments for result_of

template <auto CST, auto NST, auto Pointer >
struct unwrap_of_function<map_transition< CST, NST, Pointer>> {
  static constexpr auto END = NST;
};

template <typename map_protocol, auto CST, auto Pointer>
constexpr auto vaild_transition =
unwrap_of_function<
find_transition<CST, Pointer, map_protocol>>::END;



template <typename T, typename... Args>
struct unwrap_of_CurrentState {
  static_assert(!std::is_same_v<T, not_There>, "Current State is not defind");
};

template <auto CST, auto NST, auto Pointer, typename... Args>
struct unwrap_of_CurrentState<map_transition<CST,NST, Pointer>,
                               Args...> {
  using type = std::result_of_t<decltype(CST)(Args...)>;
};

template <typename T, typename... Args>
struct unwrap_of_NextState {
  static_assert(!std::is_same_v<T, not_There>, "Next State is not defind");
};

template <auto CST, auto NST, auto Pointer, typename... Args>
struct unwrap_of_NextState<map_transition<CST,NST, Pointer>,
                               Args...> {
  using type = std::result_of_t<decltype(NST)(Args...)>;
};



template <auto CST,  template <auto State> typename Check, typename Class, typename map_protocol>
class Checker{
public:
    template <auto NewState>
    using ThisClass = Checker<NewState, Check, Class, map_protocol>;
    // Check that the transition is valid, and thnreturn the object pointer to target state.
    template <auto Pointer, typename... Args>
      auto Check_transition(Args&&... args) && {
        (classID*Pointer)(forward<Args>(args)...);
        constexpr auto target_state =
          vaild_transition<map_protocol, CST, Pointer>;
        return Check<target_state>(
                  ThisClass<target_state>{move(classID), true});
      }
    
private:
    Class classID;

    
public:
  
 template<auto, template <auto> typename, typename, typename>
 friend class Checker;
 
   
};







}





#define Assign_to_Class(Check, CLass, type, map_protocol) \
template<type CurrentState>  \
  class Check                                                         \
    : public ::TypestateTool::Checker<                             \
          CurrentState, Check, CLass,            \
map_protocol>                                          \
  {                                                                            \
   public:                                                                     \
    using ID =                                                               \
::TypestateTool::Checker<                                  \
            CurrentState, Check,  CLass,         \
map_protocol>;                    \
    using class_id = CLass;                                              \
   protected:\
    template<auto, template <auto> typename, typename, typename  \
             >                                             \
    friend class ::TypestateTool::Checker;   \
Check(ID&& other) : Check(other) {}            \
public:                                                                     \
 Check() : ID(class_id{}) {}                                        \
 /* Disallow copy constructor. */                                           \
Check(const Check&) = delete;                                \
Check& operator=(const Check&) = delete;                                                                \
\
       \
                  \
}\


