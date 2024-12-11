#include "Student.h"

int main(int argc, char** argv) {   
    Student s;
    Student& s2=s;
    
    s.Register("ARWA","38940");
    s.Enrol("ARWA");
    s2.LogOut("Arwa");

    return 0;
};