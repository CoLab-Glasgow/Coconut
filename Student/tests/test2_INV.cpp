#include "../Student.h"

int main(){
    student s;
    (s->*&Student::Register)("Arwa", "2598556A");
    (s->*&Student::Enrol)("CourseID");
    (s->*&Student::Register)("Arwa", "2598556B"); // Invalid, attempting to register when not in UNAPPROVED state.
    return 0;

}