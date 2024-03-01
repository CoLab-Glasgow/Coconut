#include "../Sound.h"
int main(){
    sound s;
    (s->*&Sound::TurnON)(); // From OFF to ON
    // Attempting to perform two state transitions simultaneously, which the protocol likely does not support
    (s->*&Sound::TurnOff)(); // Attempting to turn OFF
    (s->*&Sound::turnHigh)(); // Attempting to turn HIGH simultaneously, which is invalid sequencing
    
    return 0;
}