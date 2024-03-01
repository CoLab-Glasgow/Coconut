#include "../Basket.h"
int main() {
    Basket basket;
    (basket->*&Basket_Class::addItemsToBasket)("Pen", 2.30); // EMPTY -> NONEMPTY
    (basket->*&Basket_Class::deleteItem)("Pen", 2.30); // NONEMPTY -> UNKNOWN
    (basket->*&Basket_Class::addItemsToBasket)("Box", 5.0); // UNKNOWN -> NONEMPTY
    (basket->*&Basket_Class::calculate)(); // NONEMPTY -> END
    return 0;
}
