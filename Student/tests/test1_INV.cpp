#include "../Student.h"

int main(){
    student s;
    (s->*&Student::Enrol)("CourseID"); // Invalid as it does not follow the UNAPPROVED -> APPROVED transition.
    return 0;
}