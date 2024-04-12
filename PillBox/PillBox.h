#include "TypestateLibrary.h"
#include <vector>
#include "Drawer.h"
#include "RedLed.h"

using TypestateLibrary::TypestateClassConnector;
using TypestateLibrary::State;
using TypestateLibrary::Typestate_Template;

class PillBox {
public:
    
    PillBox() {
      DrawersBox = new std::vector<Drawer*>();
    }
    void Activate_pillBox() {
       std::cout << "PillBox is active now!\n";
    }
    void addDrawers(Drawer* d) {
      DrawersBox->push_back(d);
    }
    Drawer* Process_System_Time(int h, int m) {
      for (Drawer* d : *DrawersBox) {
        if (h == d->get_the_hour() && m == d->get_minutes())
          {
            return d;
          }
        }
      return nullptr;
    }
    void Deactivate_Pill_Box() {
      std::cout<<"PillBox is Deactivated now!\n";
    }
    void Switch_ON(Drawer* d) { 
      redled.setRedLed("ON");
      std::cout<<"It's time to take "<<d->get_pill_name()<<"!\n";   
    }
    void Switch_OFF(Drawer* d) {
      redled.setRedLed("OFF");
      d->SetDrawerState("CLOSED");
      std::cout << "Drawer with the pill " << d->get_pill_name() << " is closed\n";

    }
    void Blink(Drawer* d) {
      redled.setRedLed("Blinking");
      std::cout << "Please close the drawer \n";
    }
    
private:
    std::vector<Drawer*>* DrawersBox;
    RedLed redled;
};

BETTER_ENUM(domain, int,
    Idle, Active, NonActive, Pill_Time_On, RedLedON, RedLedOFF, RedLedBlinking);

    using PillBox_typestate = Typestate_Template<
    State<+domain::Idle,&PillBox::Activate_pillBox,+domain::Active>,
    State<+domain::Active,&PillBox::Process_System_Time,+domain::Pill_Time_On>,
    State<+domain::Pill_Time_On,&PillBox::Switch_ON,+domain::RedLedON>,
    State<+domain::RedLedON,&PillBox::Switch_ON,+domain::RedLedON>,
    State<+domain::RedLedON,&PillBox::Blink,+domain::RedLedBlinking>,
    State<+domain::RedLedON,&PillBox::Switch_OFF,+domain::RedLedOFF>,
    State<+domain::RedLedBlinking,&PillBox::Switch_OFF,+domain::RedLedOFF>,
    State<+domain::Active,&PillBox::Deactivate_Pill_Box,+domain::NonActive>
    >;




using Pillbox = TypestateClassConnector<PillBox, PillBox_typestate>;
