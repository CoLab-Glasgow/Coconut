#include "Sound.h"

int main(int argc, const char* argv[]) {  
   sound s;
   (s->*&Sound::TurnON)();
   (s->*&Sound::turnLow)();
   (s->*&Sound::turnHigh)();
   (s->*&Sound::TurnOff)();
    return 0;
}
