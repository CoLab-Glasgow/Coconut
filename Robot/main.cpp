#include "Robot.h"

int main() {
    Robot robot;
    robot.StartNavigation();
    robot.PickUpObject("Cup");
    robot.ReleaseObject("Cup");  
    robot.Finish();
    return 0;
}
