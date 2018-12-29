#ifndef Spectrometer_H
#define Spectrometer_H

#include <string>
#include <iostream>

#include "Tool.h"

class Spectrometer: public Tool {


 public:

  Spectrometer();
  bool Initialise(std::string configfile,DataModel &data);
  bool Execute();
  bool Finalise();


 private:





};


#endif
