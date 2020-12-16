#ifndef Valve_H
#define Valve_H

#include <string>
#include <iostream>

#include "Tool.h"

class Valve: public Tool {


 public:

  Valve();
  bool Initialise(std::string configfile,DataModel &data);
  bool Execute();
  bool Finalise();


 private:

  int m_valve_pin;
  std::string valve;
  
  void ValveOpen();
  void ValveClose();


};


#endif
