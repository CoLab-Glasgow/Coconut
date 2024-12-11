#include "Basket.h"

int main(int argc, const char* argv[]) {
    Basket_Class basket1;
    basket1.addItemsToBasket("Book", 10.30);
    basket1.addItemsToBasket("pen", 2.30);
    basket1.deleteItem("Book", 10.30);
    basket1.clear();
    return 0;
}