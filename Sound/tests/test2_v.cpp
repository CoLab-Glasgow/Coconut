#include "../Sound.h"
int main(){
sound s;
    (s->*&Sound::TurnON)(); // From OFF to ON
    (s->*&Sound::turnHigh)(); // From ON to HIGH
    (s->*&Sound::TurnOff)(); // From HIGH back to OFF
    
    return 0;

}