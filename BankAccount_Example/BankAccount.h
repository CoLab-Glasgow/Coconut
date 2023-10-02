#pragma once
pragma once

#include "TypestateLibrary.h"

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

