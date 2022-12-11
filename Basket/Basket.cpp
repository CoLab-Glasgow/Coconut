//
//  Basket.cpp
//  shoppingBasket
//
//  Created by Arwa Alsubhi on 22/11/2021.
//





#include <vector>
#include <algorithm>
#include<string>
#include "TypestateLibrary.h"

using namespace std;

// create a Basket class
class Basket{
    
    // this Basket use a vector 
    std::vector<std::string> items;
    std::vector<float>  prices;
    float total=0;
 
public:
    Basket(){
        
    }
    
    // clear the vector
    void clear(){
        items.clear();
        prices.clear();
        total=0;
    }
    
    // add items to the vector
    void addItemsToBasket(string item, float price){
        
        items.push_back(item);
    
        prices.push_back(price);
        
    }
    // delete items from the vector
    void deleteItem(string item, float price){
        items.erase(std::remove(items.begin(), items.end(), item), items.end());
        prices.erase(std::remove(prices.begin(), prices.end(), price), prices.end());
    

    }
    
   // pay 
    void calculate(){
        
        for(float i : prices)
            total=total+i;
        
        cout<<"Total price for the order = "<< total;
        items.clear();
        prices.clear();
        total=0;
    }
    
    // checking the basket for emptyness 
    bool checkBasket(){
        int i=0;
        for( ; i<items.size();i++){
            
            
        }
        if (i>0){
            return true;
        }
    
        return false;
    }
   
    
    
};

// define states of the protocol 
enum class BasketState{
    EMPTY,
    NONEMPTY,
    UNKOWN,
    END,
};


using TypestateTool::State;

using TypestateTool::typestate;


// define protocol
using Basket_protocol = typestate<
    State<BasketState::EMPTY, &Basket::addItemsToBasket,BasketState::NONEMPTY>,
    State<BasketState::EMPTY, &Basket::clear, BasketState::END>,
    State<BasketState::NONEMPTY, &Basket::addItemsToBasket, BasketState::NONEMPTY>,
    State<BasketState::NONEMPTY, &Basket::deleteItem, BasketState::UNKOWN>,
    State<BasketState::UNKOWN, &Basket::addItemsToBasket, BasketState::NONEMPTY>,
    State<BasketState::UNKOWN, &Basket::clear,BasketState::EMPTY>,
    State<BasketState::NONEMPTY, &Basket::calculate,BasketState::END>
>;

// assign it to class


 Assign_to_Class(Basket, Basket_protocol);


int main(int argc, const char * argv[]) {
    
    
    // insert code here...
    
    Basket basket1 ;
    basket1.addItemsToBasket("Book", 10.30);
    basket1.addItemsToBasket("pen", 2.30);
    basket1.addItemsToBasket("box", 5.0);
    basket1.deleteItem("pen",2.30);
    basket1.calculate();
    
    
    
    return 0;
}
