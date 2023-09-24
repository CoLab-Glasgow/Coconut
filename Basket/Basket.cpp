

#include <vector>
#include <algorithm>
#include <string>
#include "TypestateLibrary.h"

using TypestateLibrary::TypestateClassConnector;
using TypestateLibrary::State;
using TypestateLibrary::Typestate_Template;

//Create a Basket class
class Basket_Class {

// this Basket uses a vector 
std::vector<std::string> items;
std::vector<float>  prices;
float total = 0;

public:
 Basket_Class() {

    }

    //Clear the vector
 void clear() {
        items.clear();
        prices.clear();
        total = 0;
    }

    //Add items to the vector
void addItemsToBasket(std::string item, float price) {

        items.push_back(item);
    prices.push_back(price);

    }
    // delete items from the vector
void deleteItem(std::string item, float price) {
        items.erase(std::remove(items.begin(), items.end(), item), items.end());
        prices.erase(std::remove(prices.begin(), prices.end(), price), prices.end());
    }

    // pay 
void calculate() {

for (float i : prices)
    total = total + i;
    std::cout << "Total price for the order = " << total;
    items.clear();
    prices.clear();
     total = 0;
    }

    // checking the basket for emptiness 
bool checkBasket() {
    int i = 0;
    for (; i < items.size(); i++) {
        }
    if (i > 0) {
    return true;
        }
        return false;
    }



};

//Define states of the protocol 
BETTER_ENUM(BasketState , int,
    EMPTY,
    NONEMPTY,
    UNKNOWN,
    END
)


//Define protocol
using Basket_protocol = Typestate_Template<
    State<BasketState::EMPTY, &Basket_Class::addItemsToBasket, BasketState::NONEMPTY>,
    State<BasketState::EMPTY, &Basket_Class::clear, BasketState::END>,
    State<BasketState::NONEMPTY, &Basket_Class::addItemsToBasket, BasketState::NONEMPTY>,
    State<BasketState::NONEMPTY, &Basket_Class::deleteItem, BasketState::UNKOWN>,
    State<BasketState::UNKOWN, &Basket_Class::addItemsToBasket, BasketState::NONEMPTY>,
    State<BasketState::UNKOWN, &Basket_Class::clear, BasketState::EMPTY>,
    State<BasketState::NONEMPTY, &Basket_Class::calculate, BasketState::END>
>;


using Basket = TypestateClassConnector<Basket_Class, Basket_protocol>;

int main(int argc, const char* argv[]) {

    Basket basket1;
    
    (basket1->*&Basket_Class::addItemsToBasket)("Book", 10.30);
    (basket1->*&Basket_Class::addItemsToBasket)("pen", 2.30);
    (basket1->*&Basket_Class::addItemsToBasket)("box", 5.0);
    (basket1->*&Basket_Class::deleteItem)("pen", 2.30);
    (basket1->*&Basket_Class::calculate)();



    return 0;
}
