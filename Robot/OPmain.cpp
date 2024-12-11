#include "Operator.h"

int main() {
Robot robot;
Operator operator1;
robot.StartNavigation("shelf03901");
operator1.automatePickingReleasing(robot);
robot.Finish();

return 0;
}