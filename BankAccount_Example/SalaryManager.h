#include "BankAccount.h"
#include "DataStorage.h"
#include "TypestateLibrary.h"

using TypestateLibrary::TypestateClassConnector;
using TypestateLibrary::State;
using TypestateLibrary::Typestate_Template;





class SalaryManager {
public:

void setMoney(BankAccount* m) {
        money = m;
    }

void addSalary(float amount) {
        money->fill(amount);
        money->applyInterest(1.02f);
    }

void Display() {
        std::cout << "Information For Account" << std::endl;
    }

private:

BankAccount* money = nullptr;

};


BETTER_ENUM(BankStates, int, INIT= 0, INTERMEDITE, FILLED, END)


using BankProtocol= Typestate_Template<
    State<BankStates::INIT, &SalaryManager::setMoney, BankStates::INTERMEDITE>,
    State<BankStates::INIT, &DataStorage::setMoney, BankStates::FILLED>,
    State<BankStates::INTERMEDITE, &SalaryManager::addSalary, BankStates::FILLED>,
    State<BankStates::FILLED, &DataStorage::store, BankStates::END>> ;


using Account = TypestateClassConnector<BankAccount, BankProtocol> ;
using Manager = TypestateClassConnector<SalaryManager, BankProtocol> ;
using Storge = TypestateClassConnector<DataStorage, BankProtocol> ;





