#ifndef MeasurementManager_H
#define MeasurementManager_H

#include <string>
#include <iostream>

#include "Tool.h"

class MeasurementManager: public Tool {


 public:

  MeasurementManager();
  bool Initialise(std::string configfile,DataModel &data);
  bool Execute();
  bool Finalise();


 private:





};


#endif
