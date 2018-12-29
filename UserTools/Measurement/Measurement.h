#ifndef Measurement_H
#define Measurement_H

#include <string>
#include <iostream>

#include "Tool.h"

class Measurement: public Tool {


 public:

  Measurement();
  bool Initialise(std::string configfile,DataModel &data);
  bool Execute();
  bool Finalise();


 private:





};


#endif
