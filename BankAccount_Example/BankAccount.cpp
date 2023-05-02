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
