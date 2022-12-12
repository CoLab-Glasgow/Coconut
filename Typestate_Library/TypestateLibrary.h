

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





namespace TypestateTool{
 

// create a state template
template<auto state, auto FP, auto NextState>
 struct State;
// create typestates tempalate to store all states and transitions
template <typename...State>
struct typestate;


 
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


 
 template <auto state, auto FP,
        typename... typestate>
    struct Unwrap_typestate {
        // just simple intilization 
    };





    template <auto state, auto FP, auto NextState, typename... typestate>
    struct Unwrap_typestate <state, FP, State<state, FP, NextState>,
        typestate...> {
        using type = State<FP, state, NextState>;

    };

    template <auto state, auto FP, typename State,
        typename... typestate>
    struct  Unwrap_typestate <state, FP, State,
        typestate...> {
        using type = typename Unwrap_typestate<state, FP,
            typestate...>::type;
    };


    template <auto state, auto FP, typename... State>
    struct Unwrap_typestate<state, FP,
        typestate<State...>> {
        using type = typename Unwrap_typestate<state, FP,
            State...>::type;
    };

    template <typename T, typename... Args >
    struct return_of_FB {};


    template <auto state, auto FP, auto NextState, typename... Args>
    struct return_of_FB<State<state, FP, NextState>,
        Args...> {
        using type = std::invoke_result<decltype(FP)(Args...)>;

    };



    template <typename T, typename... Args >
    struct return_of_State {
    };

    template <auto state, auto FP, auto NextState, typename... Args>
    struct return_of_State<State<state, FP, NextState>,
        Args...> {
        using type = std::invoke_result<decltype(state)(Args...) >;

    };

 

template <class T>
    struct is_reference_wrapper : std::false_type {};


  template<typename T, typename... TT>
    struct All_states {
        using next = All_states<TT...>;
        static const constexpr std::size_t size = 1 + next::size;
        std::vector<T> typestateVector;
        template<typename C>
        inline constexpr static C for_each(C cbk, T&& tval, TT &&... ttval) {
            cbk(std::forward<T>(tval));
            next::for_each(cbk, std::forward<TT>(ttval)...);

            return cbk;
        }

        template<typename C>
        inline constexpr C operator()(C cbk, T&& tval, TT &&... ttval) const {
            return for_each(cbk, std::forward<T>(tval), std::forward<TT>(ttval)...);
        }
    };



// this checker will use templates above to check protocol validation



    
    

    
   
template<typename typestate>
    class TypestateChecker {
    public:


        using checker = TypestateChecker<typestate>;

        using size = std::size_t(typestate);



        template <auto FB, typename... Args>
        auto  save_typestate(Args&&... args)&& {
            
                  

            //auto F_b = unw
          // save_typestate(FB, Args...)(std::forward<Args>(args)...);
            (checker*FB)(std::forward<Args>(args)...);
            auto target_FB = (return_of_FB<typestate,FB>);
            //auto target_state = (return_of_state<typestate, state, FB>);





            return target_FB;
        }


    protected:






    private:
        TypestateChecker cheker;







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






