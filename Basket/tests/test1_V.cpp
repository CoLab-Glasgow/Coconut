#include "../Basket.h"
int main() {
    Basket basket;
    (basket->*&Basket_Class::addItemsToBasket)("Book", 10.30); // EMPTY -> NONEMPTY
    (basket->*&Basket_Class::clear)(); // NONEMPTY -> END (assuming clearing a NONEMPTY basket leads to END)
    return 0;
}
