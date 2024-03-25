#include "Student.h"

int main(int argc, char** argv) {   
    student s;
    (s->*&Student::Register)("Arwa","2598556A");
    (s->*&Student::Enrol)("Arwa");
    (s->*&Student::LogOut)("Arwa");
    return 0;
};
