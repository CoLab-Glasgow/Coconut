#include "Basket.h"
int main(int argc, const char* argv[]) {
    Basket basket1;
    (basket1->*&Basket_Class::addItemsToBasket)("Book", 10.30);
    (basket1->*&Basket_Class::addItemsToBasket)("pen", 2.30);
    (basket1->*&Basket_Class::addItemsToBasket)("box", 5.0);
    (basket1->*&Basket_Class::deleteItem)("pen", 2.30);
    (basket1->*&Basket_Class::calculate)();
    return 0;
}