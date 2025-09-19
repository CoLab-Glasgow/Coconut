#include <thread>
#include "Operator.h"

int main() {
    Robot robot1;
    std::thread t1([&]() {
        std::cout<<"T1 ..\n";
        robot1.StartNavigation("shelf03901");
    });
    std::thread t2([&]() {
          std::cout<<"T2 ..\n";
        robot1.PickUpObject("d");
    });
    t1.join();
    t2.join();
    return 0;
}
