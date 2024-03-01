#include "../Student.h"
int main(){
     student s;
    (s->*&Student::Register)("Arwa", "2598556A");
    (s->*&Student::Enrol)("CourseID1");
    (s->*&Student::Enrol)("CourseID2"); // Remains in ENROLLED, valid operation.
}