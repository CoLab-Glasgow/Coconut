#include "../Sound.h"

int main(){
     sound s;
    (s->*&Sound::TurnOff)(); // Invalid because it's not ON yet
    return 0;
}