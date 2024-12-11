#include <iostream>

class RedLed {
public:
void setRedLed(const char* state) {
    this->RedLedState = state;
}
private:
    std::string RedLedState;
};
