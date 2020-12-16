#ifndef BenPower_H
#define BenPower_H

#include <string>
#include <iostream>
#include <stdlib.h>
#include <sstream>
#include "Tool.h"

class BenPower: public Tool {
  
  
public:
  
  BenPower();
  bool Initialise(std::string configfile,DataModel &data);
  bool Execute();
  bool Finalise();
  
  void TurnOn();
  void TurnOff();
  
private:
  
  int m_powerpin;
  std::string power;  
  
  
};


#endif
