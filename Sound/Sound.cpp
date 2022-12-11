//
//  main.cpp
//  Light
//
//  Created by Arwa Alsubhi on 29/11/2021.
//





#include "TypestateLibrary.h"
using namespace std;
// define states of the protocol 
// could be defined anywhrer in the program
enum class StatesOfSound{
    OFF,
    ON,
    HIGH,
    LOW,
};
// create sound class
class Sound{
    
    
public:
    Sound()=default;
    
    void TurnON(){
        cout<<"Sound is On"<<endl;
    }
    
    void TurnOff(){
        cout<<"Sound is Off"<<endl;
    }
    
    void turnHigh(){
        cout<<"Sound is High"<<endl;
    }
    void turnLow(){
        cout<<"Sound is Low"<<endl;
    }
    
    
};


// exctract the templates
using TypestateTool::State;

using TypestateTool::typestate;

// define the protocol

using Sound_protocol = typestate< 
    State<StatesOfSound::OFF, &Sound::TurnON, StatesOfSound::ON>,
    State<StatesOfSound::ON, &Sound::turnHigh, StatesOfSound::HIGH>,
    State<StatesOfSound::ON, &Sound::turnLow, StatesOfSound::LOW>,
    State<StatesOfSound::ON, &Sound::TurnOff, StatesOfSound::OFF>,
    State<StatesOfSound::LOW, &Sound::turnHigh, StatesOfSound::HIGH>,
    State<StatesOfSound::HIGH, &Sound::turnLow,StatesOfSound::LOW>,
    State<StatesOfSound::LOW, &Sound::TurnOff,StatesOfSound::OFF>,
    State<StatesOfSound::HIGH, &Sound::TurnOff,StatesOfSound::OFF> 

>;
// assign to class

Assign_to_Class(Sound,Sound_protocol);

int main(int argc, const char * argv[]) {
    // insert code here...
    Sound s;
    s.TurnON();
    s.turnLow();
    s.turnHigh();
    s.TurnOff();
    
   
    return 0;
}
