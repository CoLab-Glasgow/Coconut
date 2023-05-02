#include "Bank.h"
#include "Coconut/TypestateLibrary.h"
#include <iostream>

using TypestateLibrary::Typestate_Checker;
using TypestateLibrary::State;
using TypestateLibrary::Typestate_Template;



class BankAccount {
public:
    float get() const {
        return balance;
    }

    void fill(float amount) {
        balance += amount;
    }

    void applyInterest(float rate) {
        balance *= rate;
    }

private:
    float balance = 0.0f;
};


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
