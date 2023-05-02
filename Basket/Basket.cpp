
#include "Basket_Class.h"
//
//  Basket.cpp
//  shoppingBasket
//
//  Created by Arwa Alsubhi on 22/11/2021.
//





#include <vector>
#include <algorithm>
#include<string>
#include "Cocount\TypestateLibrary.h""
using TypestateLibrary::Typestate_Checker;
using TypestateLibrary::State;
using TypestateLibrary::Typestate_Template;

// create a Basket class
class Basket_Class {

    // this Basket use a vector 
    std::vector<std::string> items;
    std::vector<float>  prices;
    float total = 0;

public:
    Basket_Class() {

    }

    // clear the vector
    void clear() {
        items.clear();
        prices.clear();
        total = 0;
    }

    // add items to the vector
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

    // checking the basket for emptyness 
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

// define states of the protocol 
BETTER_ENUM(BasketState , int,
    EMPTY,
    NONEMPTY,
    UNKOWN,
    END
)


// define protocol
using Basket_protocol = Typestate_Template<
    State<BasketState::EMPTY, &Basket_Class::addItemsToBasket, BasketState::NONEMPTY>,
    State<BasketState::EMPTY, &Basket_Class::clear, BasketState::END>,
    State<BasketState::NONEMPTY, &Basket_Class::addItemsToBasket, BasketState::NONEMPTY>,
    State<BasketState::NONEMPTY, &Basket_Class::deleteItem, BasketState::UNKOWN>,
    State<BasketState::UNKOWN, &Basket_Class::addItemsToBasket, BasketState::NONEMPTY>,
    State<BasketState::UNKOWN, &Basket_Class::clear, BasketState::EMPTY>,
    State<BasketState::NONEMPTY, &Basket_Class::calculate, BasketState::END>
>;

// assign it to class



using Basket = Typestate_Checker<Basket_Class, Basket_protocol>;

int main(int argc, const char* argv[]) {


    // insert code here...

    Basket basket1;
    (basket1->*&Basket_Class::addItemsToBasket)("Book", 10.30);
    (basket1->*&Basket_Class::addItemsToBasket)("pen", 2.30);
    (basket1->*&Basket_Class::addItemsToBasket)("box", 5.0);
    (basket1->*&Basket_Class::deleteItem)("pen", 2.30);
    (basket1->*&Basket_Class::calculate)();



    return 0;
}
