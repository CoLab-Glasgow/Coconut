//
//  main.cpp
//  Light
//
//  Created by Arwa Alsubhi 


#include "TypestateLibrary.h"

using TypestateLibrary::TypestateClassConnector;
using TypestateLibrary::State;
using TypestateLibrary::Typestate_Template;



// define states of the protocol 
// could be defined anywhrer in the program
enum  StatesOfSound {
    OFF,
    ON,
    HIGH,
    LOW,
};
// create sound class
class Sound {


public:
    Sound() = default;

    void TurnON() {
        std::cout << "Sound is On" << std::endl;
    }

    void TurnOff() {
        std::cout << "Sound is Off" << std::endl;
    }

    void turnHigh() {
        std::cout << "Sound is High" << std::endl;
    }
    void turnLow() {
        std::cout << "Sound is Low" << std::endl;
    }


};




// define the protocol

using Sound_protocol = Typestate_Template<
    State<StatesOfSound::OFF, &Sound::TurnON, StatesOfSound::ON>,
    State<StatesOfSound::ON, &Sound::turnHigh, StatesOfSound::HIGH>,
    State<StatesOfSound::ON, &Sound::turnLow, StatesOfSound::LOW>,
    State<StatesOfSound::ON, &Sound::TurnOff, StatesOfSound::OFF>,
    State<StatesOfSound::LOW, &Sound::turnHigh, StatesOfSound::HIGH>,
    State<StatesOfSound::HIGH, &Sound::turnLow, StatesOfSound::LOW>,
    State<StatesOfSound::LOW, &Sound::TurnOff, StatesOfSound::OFF>,
    State<StatesOfSound::HIGH, &Sound::TurnOff, StatesOfSound::OFF>

>;
// assign to class

using sound = TypestateClassConnector<Sound, Sound_protocol>;

int main(int argc, const char* argv[]) {
    // insert code here...
    sound s;
    (s->*&Sound::TurnON)();
    (s->*&Sound::turnLow)();
    (s->*&Sound::turnHigh)();
    (s->*&Sound::TurnOff)();


 


    return 0;
}
