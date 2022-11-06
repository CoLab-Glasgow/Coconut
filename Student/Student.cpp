
/*
 * File:   main.cpp
 * Author: arwaalsubhi
 *
 * Created on 12 October 2021, 16:19
 */


/*
 * File:   main.cpp
 * Author: arwaalsubhi
 *
 * Created on 12 October 2021, 16:19
 */

#include <cstdlib>
#include<iostream>
#include<string>
#include "TypestateLibrary.h"



using namespace std;

// define Student class

class Student {
public:
    Student()=default;
string Name;
string studentID;
    
void Register(){
  cout<<Name<< " has been registered with ID" <<studentID<<endl;
   }
void Enrol(){
  cout<<Name<< "has been enrolled in a course" << endl;
}
void LogOut(){
   cout<<Name<<"has Logged out"<<endl;
  }
};

using TypestateTool::State;
using TypestateTool::typestates;
using TypestateTool::transition;


enum class Studentstate {
    UNAPPROVED,
    APPROVED,
    ENROLLED,
    END,
};


using StudentProtocol = typestates<
    State<Studentstate::UNAPPROVED, transition
    <&Student::Register, Studentstate::APPROVED>>,
    State<Studentstate::APPROVED,
    transition<&Student::Enrol, Studentstate::ENROLLED>>,

    State<Studentstate::ENROLLED,
    transition<&Student::Enrol, Studentstate::ENROLLED>,
    transition<&Student::LogOut, Studentstate::END> >



    >;

// assign it and link it class
Assign_to_Class(Student, StudentProtocol);





int main(int argc, char** argv) {
    // create object of the class
    Student s ;
    
   
    s.Register();
    s.Enrol();
    s.Enrol();
    s.LogOut();
    
    return 0;
};







   
