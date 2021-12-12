//
//  main.cpp
//  Light
//
//  Created by Arwa Alsubhi on 29/11/2021.
//




#include <iostream>
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
using TypestateTool::map_transition;

using TypestateTool::map_protocol;

// define the protocol

using Sound_protocol=  map_protocol< map_transition<StatesOfSound::OFF , StatesOfSound::ON, &Sound::TurnON>,
map_transition<StatesOfSound::ON , StatesOfSound::HIGH, &Sound::turnHigh>,
map_transition<StatesOfSound::ON , StatesOfSound::LOW, &Sound::turnLow>,
map_transition<StatesOfSound::ON , StatesOfSound::OFF, &Sound::TurnOff>,
map_transition<StatesOfSound::LOW , StatesOfSound::HIGH, &Sound::turnHigh>,
map_transition<StatesOfSound::HIGH , StatesOfSound::LOW, &Sound::turnLow>,
map_transition<StatesOfSound::LOW , StatesOfSound::OFF, &Sound::TurnOff>,
map_transition<StatesOfSound::HIGH , StatesOfSound::OFF, &Sound::TurnOff>

> ;
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
