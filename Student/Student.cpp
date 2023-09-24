

/*
 * File:   main.cpp
 * Author: arwaalsubhi
 *
 * 
 */


#include <cstdlib>
#include<iostream>
#include<string>
#include "TypestateLibrary.h"

using TypestateLibrary::TypestateClassConnector;
using TypestateLibrary::State;
using TypestateLibrary::Typestate_Template;



// define Student class

class Student {
public:
    Student() {
   

    }
   

    void Register(std::string n , std::string i) {
        std::cout << n << " has been registered with ID" << i << std::endl;
    }
    void Enrol(std::string n) {
        std::cout << n << "has been enrolled in a course" << std::endl;
    }
    void LogOut(std::string n) {
        std::cout << n << "has Logged out" << std::endl;
    }
  
        

};





BETTER_ENUM (Studentstate, int, 
    UNAPPROVED,
    APPROVED,
    ENROLLED,
    END
    )


using StudentProtocol = Typestate_Template<

    State<Studentstate::UNAPPROVED, &Student::Register, Studentstate::APPROVED>,
    State<Studentstate::APPROVED, &Student::Enrol, Studentstate::ENROLLED>,
    State<Studentstate::ENROLLED, &Student::Enrol, Studentstate::ENROLLED>,
    State<Studentstate::END, &Student::LogOut, Studentstate::END>
>;


using student = TypestateClassConnector<Student, StudentProtocol>;


int main(int argc, char** argv) {
    
    student s;

    (s->*&Student::Register)("Arwa","2598556A");
    (s->*&Student::Enrol)("Arwa");
    (s->*&Student::Register)("Arwa", "2598556");


    return 0;
};


