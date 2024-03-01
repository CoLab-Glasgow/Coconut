#include "../Basket.h"

int main() {
    Basket basket;
    (basket->*&Basket_Class::deleteItem)("Book", 10.30); // Attempting to delete from EMPTY, which is not a defined transition
    return 0;
}
