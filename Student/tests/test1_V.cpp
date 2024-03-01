#include "../Student.h"

int main(){
    student s;
    (s->*&Student::Register)("Arwa", "2598556A"); // Assuming this successfully transitions from UNAPPROVED to APPROVED
    (s->*&Student::Enrol)("CourseID"); // Successfully transitions from APPROVED to ENROLLED
    return 0;
}