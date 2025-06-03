#include <cstdlib>
#include <iostream>

#include "../include/Typestate_Library/TypestateLibrary.h"

using TypestateLibrary::Template::TypestateClassConnector;
using TypestateLibrary::Template::State;
using TypestateLibrary::Template::Typestate_Template;

class Buzzer {
public:
  void TurnOn()  { std::cout << "[Buzzer] Turning ON\n";  /* … */ }
  void TurnOff() { std::cout << "[Buzzer] Turning OFF\n"; /* … */ }
};

enum class BuzzerState {
    OFF = 0,
    ON  = 1
};

using BuzzerProtocol = Typestate_Template<
    State<BuzzerState::OFF, &Buzzer::TurnOn, BuzzerState::ON>,
    State<BuzzerState::ON,  &Buzzer::TurnOff, BuzzerState::OFF>
>;

TypestateClassConnector< Buzzer, BuzzerProtocol > buzzerflag;
