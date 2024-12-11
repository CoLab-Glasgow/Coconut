#include <cstdlib>
#include <iostream>
#include <string>
#include "../include/Typestate_Library/TypestateLibrary.h"

using TypestateLibrary::Template::TypestateClassConnector;
using TypestateLibrary::Template::State;
using TypestateLibrary::Template::Typestate_Template;


class Student {
public:
    
    void Register (const char* n , const char* id) {
       printf("Registering student: Name = %s, ID = %s\n", n, id);
    }
    void Enrol(const char* n) {
        printf("Enrolling student in: %s\n", n);
    }
    void LogOut(const char* n) {
       printf("Logging out student: %s\n", n);
    }    
    

};



enum class Studentstate { 
    UNAPPROVED =0,
    APPROVED=1,
    ENROLLED=2,
    END=3
};


using StudentProtocol = Typestate_Template<
    State<Studentstate::UNAPPROVED, &Student::Register, Studentstate::APPROVED>,
    State<Studentstate::APPROVED, &Student::Enrol, Studentstate::ENROLLED>,
    State<Studentstate::ENROLLED, &Student::Enrol, Studentstate::ENROLLED>,
    State<Studentstate::ENROLLED, &Student::LogOut, Studentstate::END>
>;

using student_Flag = TypestateClassConnector<Student, StudentProtocol>;

student_Flag StudentFlag;

void init(){
    StudentProtocol t;
    t.display();
    StudentFlag.display();
}