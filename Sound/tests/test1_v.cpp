#include "../Sound.h"
int main(){
 sound s;
    (s->*&Sound::TurnON)(); // From OFF to ON
    (s->*&Sound::TurnOff)(); // From ON back to OFF
    
    return 0;

}