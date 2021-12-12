
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

// define States of transitions 
enum class Studentstate{
   UNAPPROVED,
   APPROVED,
   ENROLLED,
   END,
};




// exctract the templates
using TypestateTool::map_transition;
using TypestateTool::map_protocol;



// define the protocol
 
using Student_protocol=  map_protocol<
map_transition<Studentstate::UNAPPROVED, Studentstate::APPROVED, &Student::Register>,
map_transition<Studentstate::APPROVED,Studentstate::ENROLLED, &Student::Enrol>,
map_transition<Studentstate::ENROLLED,Studentstate::ENROLLED, &Student::Enrol>,
 map_transition<Studentstate::ENROLLED, Studentstate::END, &Student::LogOut> >;

// assign it and link it class
Assign_to_Class(Student, Student_protocol);





int main(int argc, char** argv) {
    // create object of the class
    Student s ;
    
   
    s.Register();
    s.Enrol();
    s.Enrol();
    s.LogOut();
    return 0;
};







   
