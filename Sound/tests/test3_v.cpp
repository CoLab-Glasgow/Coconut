#include "../Sound.h"
int main(){
    sound s;
    (s->*&Sound::TurnON)(); // From OFF to ON
    (s->*&Sound::turnLow)(); // From ON to LOW
    (s->*&Sound::turnHigh)(); // From LOW to HIGH
    (s->*&Sound::TurnOff)(); // From HIGH back to OFF
    return 0;
}