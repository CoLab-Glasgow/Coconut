#include "BankAccount.h"


class DataStorage {
public:

void setMoney(BankAccount* m) {
        money = m;
    }

void store() {
        float amount = money->get();
        std::cout << amount << std::endl;
    }

private:
    BankAccount* money = nullptr;
};
