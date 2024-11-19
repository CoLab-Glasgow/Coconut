#include "TypestateLibrary.h"

#include <iostream>
#include <string>

using TypestateLibrary::State;
using TypestateLibrary::Typestate_Template;
using TypestateLibrary::TypestateClassConnector;


enum class RobotState {
    Idle,
    Navigating,
    Grip,
    Release
};

class Robot {
public:
    void StartNavigation() {
        std::cout << "Start navigation...\n";
    }
    void PickUpObject(const char* object) {   
        std::cout << "Arrived at destination. Picking up object...\n";  
    }
    void ReleaseObject(const char* object) {
        std::cout << "Object loaded. Moving to unload...\n";
    }
    void Finish() {
        std::cout << "Task complete. Returning to Idle...\n";    
    }   
};

using Robot_typestate = Typestate_Template<
    State<RobotState::Idle, &Robot::StartNavigation, RobotState::Navigating>,
    State<RobotState::Navigating, &Robot::PickUpObject, RobotState::Grip>,
    State<RobotState::Grip, &Robot::ReleaseObject, RobotState::Release>,
    State<RobotState::Release, &Robot::Finish, RobotState::Idle>>;

using Robot_Warpped= TypestateClassConnector<Robot, Robot_typestate>;