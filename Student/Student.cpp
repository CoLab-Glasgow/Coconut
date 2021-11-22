
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



class Student {
    

public:
    
  string Name;
string studentID;



Student(string n, string id){
     Name=n;
    
     studentID =id;
}

void REG(){
   
    
  cout<<Name<< " has been registered with ID : " << studentID<<endl;
   

}
void ENR(){
    
     
  cout<<Name<< " has been enrolled in courses" << endl;
 
}

void END(){
   
  cout<<Name<< " has been logged out" << endl;
  
 
  }


};
   



 enum class Studentstate{
     
    UNAPPROVED,
    APPROVED,
    ENROLLED,
    END,
};

using TypestateTool::map_transition;
using TypestateTool::map_Repeated_transition;
using TypestateTool::map_protocol;
using TypestateTool::assign_to_class;


 // defined the protocol
 
using Student_protocol=  map_protocol<map_transition<Studentstate::UNAPPROVED, Studentstate::APPROVED, &Student::REG>,map_Repeated_transition<Studentstate::APPROVED,Studentstate::ENROLLED, &Student::ENR>
, map_transition<Studentstate::ENROLLED, Studentstate::END, &Student::END>>;
// assign it to class


 using ID= assign_to_class<Student, Student_protocol>;
 
int main(int argc, char** argv) {
    
    Student s= Student("Arwa", "2598556a");

    
    s.REG();
 
    s.ENR();
    

     s.END();
     
    
    return 0;
}


   
