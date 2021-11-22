//
//  Basket.cpp
//  shoppingBasket
//
//  Created by Arwa Alsubhi on 22/11/2021.
//

#include <stdio.h>
#include <iostream>
#include <string.h>
#include <vector>
#include <algorithm>
#include "TypestateLibrary.h"

using namespace std;
class Basket{
    std::vector<std::string> items;
    std::vector<float>  prices;
    float total=0;
 
public:
    Basket(){
        
    }
    void clear(){
        items.clear();
        prices.clear();
        total=0;
    }
    void addItemsToBasket(string item, float price){
        
        items.push_back(item);
    
        prices.push_back(price);
        
    }
    
    void deleteItem(string item, float price){
        items.erase(std::remove(items.begin(), items.end(), item), items.end());
        prices.erase(std::remove(prices.begin(), prices.end(), price), prices.end());
    

    }
    
    
    void calculate(){
        
        for(float i : prices)
            total=total+i;
        
        cout<<"Total price for the order = "<< total;
        items.clear();
        prices.clear();
        total=0;
    }
    
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


enum class BasketState{
    EMPTY,
    NONEMPTY,
    UNKOWN,
    END,
};
using TypestateTool::map_transition;
using TypestateTool::map_Repeated_transition;
using TypestateTool::map_protocol;
using TypestateTool::assign_to_class;


using Basket_protocol=  map_protocol<
map_transition<BasketState::EMPTY, BasketState::NONEMPTY,&Basket::addItemsToBasket>, map_transition<BasketState::EMPTY,BasketState::END,&Basket::clear>
, map_transition<BasketState::NONEMPTY, BasketState::NONEMPTY, &Basket::addItemsToBasket>,
map_transition<BasketState::NONEMPTY, BasketState::UNKOWN, &Basket::deleteItem>,
map_transition<BasketState::UNKOWN, BasketState::NONEMPTY, &Basket::addItemsToBasket>,
map_transition<BasketState::UNKOWN, BasketState::EMPTY, &Basket::clear>,

map_transition<BasketState::NONEMPTY, BasketState::END, &Basket::calculate>
> ;



// assign it to class
 using ID= assign_to_class<Basket, Basket_protocol>;


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
