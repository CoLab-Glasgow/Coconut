#include <iostream>
class RedLed {
public:
void setRedLed(std::string state) {
    this->RedLedState = state;
}
private:
    std::string RedLedState;
};
