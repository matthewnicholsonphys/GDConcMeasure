#ifndef CalibrationManager_H
#define CalibrationManager_H

#include <string>
#include <iostream>

#include "Tool.h"

class CalibrationManager: public Tool {


 public:

  CalibrationManager();
  bool Initialise(std::string configfile,DataModel &data);
  bool Execute();
  bool Finalise();


 private:





};


#endif
