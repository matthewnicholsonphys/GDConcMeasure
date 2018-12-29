#ifndef Calibration_H
#define Calibration_H

#include <string>
#include <iostream>

#include "Tool.h"

class Calibration: public Tool {


 public:

  Calibration();
  bool Initialise(std::string configfile,DataModel &data);
  bool Execute();
  bool Finalise();


 private:





};


#endif
