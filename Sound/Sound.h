#include "../include/Typestate_Library/TypestateLibrary.h"

using TypestateLibrary::Template::State;
using TypestateLibrary::Template::Typestate_Template;
using TypestateLibrary::Template::TypestateClassConnector;

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

enum  class StatesOfSound {
    OFF,
    ON,
    HIGH,
    LOW,
};

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

using sound_flag = TypestateClassConnector<Sound, Sound_protocol>;

sound_flag sounflag;
Sound_protocol sp;
void init(){
sp.display();
sounflag.display();
}