#pragma once
#include <iostream>
#include <functional>
#include <map>
#include <cassert>
#include <stdexcept>
#include <fstream>
#include <sstream>
#include <chrono>
#include <mutex>
#include <utility>
#include <optional>
#include <set>
#include <functional>
#include <string>
#include <tuple>      
#include <algorithm>  
#include <type_traits>

using namespace std::chrono_literals;

namespace TypestateLibrary {

namespace Template {

template<auto state, auto FP, auto NextState>
struct State{
   void display() {}
};


template<typename TFirst, typename... TRest>
struct Typestate_Template{
void display() {
TFirst t;
t.display();
if constexpr (sizeof...(TRest) > 0) { 
Typestate_Template<TRest...> rest;
  rest.display();
    }
    }
};
        
template<typename T, typename Typestate>
struct TypestateClassConnector {
 void display() {}
    };

    }  
}
