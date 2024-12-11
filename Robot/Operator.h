#include "Robot.h"

class Operator{
    public:
    void automatePickingReleasing(Robot& robot){
        robot.PickUpObject("Box1837");
        robot.ReleaseObject("shelf03901","Box1837");

    }
};
