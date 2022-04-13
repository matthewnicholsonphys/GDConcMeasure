#ifndef LivePlotter_H
#define LivePlotter_H

#include <string>
#include <iostream>

#include "Tool.h"

class LivePlotter: public Tool {
  
  public:
  
  LivePlotter();
  bool Initialise(std::string configfile,DataModel &data);
  bool Execute();
  bool Finalise();
  
  private:
  int verbosity=1;
  
};


#endif
