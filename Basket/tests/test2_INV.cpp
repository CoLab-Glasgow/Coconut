#include "../Basket.h"

int main() {
    Basket basket;
    (basket->*&Basket_Class::calculate)(); // Attempting to calculate on EMPTY, which is not a defined transition to END
    return 0;
}
