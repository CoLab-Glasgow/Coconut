#include <vector>
#include <algorithm>
#include <string>
#include "../include/Typestate_Library/TypestateLibrary.h"

using TypestateLibrary::Template::TypestateClassConnector;
using TypestateLibrary::Template::State;
using TypestateLibrary::Template::Typestate_Template;


class Basket_Class { 
   std::vector<std::string> items;
   std::vector<float>  prices;
   float total = 0;
public:
    Basket_Class() { }

    void clear() {
        items.clear();
        prices.clear();
        total = 0;
    }
    void addItemsToBasket(std::string item, float price) {
        items.push_back(item);
        prices.push_back(price);
    }
    
    void deleteItem(std::string item, float price) {
        items.erase(std::remove(items.begin(), items.end(), item), items.end());
        prices.erase(std::remove(prices.begin(), prices.end(), price), prices.end());
    }
    void calculate() {
        for (float i : prices)
           total = total + i;
        std::cout << "Total price for the order = " << total;
        items.clear();
        prices.clear();
        total = 0;
    }
    bool checkBasket() {
        int i = 0;
        for (; i < items.size(); i++) {  }
       if (i > 0) {
         return true;
        }
         return false;
    }
};

enum BasketState{
    EMPTY,
    NONEMPTY,
    UNKNOWN,
    END
};

using Basket_protocol = Typestate_Template<
    State<BasketState::EMPTY, &Basket_Class::addItemsToBasket, BasketState::NONEMPTY>,
    State<BasketState::NONEMPTY, &Basket_Class::addItemsToBasket, BasketState::NONEMPTY>,
    State<BasketState::NONEMPTY, &Basket_Class::deleteItem, BasketState::UNKNOWN>,
    State<BasketState::UNKNOWN, &Basket_Class::addItemsToBasket, BasketState::NONEMPTY>,
    State<BasketState::UNKNOWN, &Basket_Class::clear, BasketState::EMPTY>,
    State<BasketState::NONEMPTY, &Basket_Class::calculate, BasketState::END>
>;

using BasketFlag = TypestateClassConnector<Basket_Class, Basket_protocol>;


BasketFlag BFlag;
Basket_protocol Bp;
void init(){
BFlag.display();
Bp.display();
}
