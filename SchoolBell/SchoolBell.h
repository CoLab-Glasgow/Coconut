#include <cstdio>
#include <cstdlib>

// Adjust this path as needed for your project structure:
#include "../include/Typestate_Library/TypestateLibrary.h"

using TypestateLibrary::Template::TypestateClassConnector;
using TypestateLibrary::Template::State;
using TypestateLibrary::Template::Typestate_Template;


class SchoolBell {
public:
    bool notify;
    bool error;
    bool mechanismRecovered;

    SchoolBell()
        : notify(false), error(false), mechanismRecovered(false)
    {
        printf("Initialized SchoolBell (OFF)\n");
    }

    void TurnOn() {
        printf("TurnOn\n"); 
    }

    void TurnOff() {
        printf("TurnOff\n");  
    }

    // Returns true if it should beep; false if it should trigger fault.
   
    bool checkStatus() {
        printf("checkStatus\n");
        return notify;
    }
    void Beep() {
        printf("Beep\n");  
    }
    void StopBeep() {
        printf("StopBeep\n");
    }
    void TriggerFault() {
        printf("TriggerFault\n");
    }
    void Reset() {
        mechanismRecovered = true;
        printf("Reset (recovered)\n");   
    }
};

enum class BellState {
    OFF  ,
    ON ,
    BEEPING ,
    FAULT  
};


using BellProtocol = Typestate_Template<
    State< BellState::OFF,  &SchoolBell::TurnOn,     BellState::ON>,
    State< BellState::ON,    &SchoolBell::TurnOff,      BellState::OFF>,
    State< BellState::ON,  &SchoolBell::checkStatus,  BellState::ON>,
    State< BellState::ON,      &SchoolBell::Beep,         BellState::BEEPING>,
    State< BellState::BEEPING, &SchoolBell::StopBeep,      BellState::ON>,
    State< BellState::ON,      &SchoolBell::TriggerFault,  BellState::FAULT>,
    State< BellState::FAULT,   &SchoolBell::Reset,        BellState::ON>
>;

TypestateClassConnector<SchoolBell, BellProtocol> bellfalg;

