#ifndef SaveTraces_H
#define SaveTraces_H

#include <string>
#include <iostream>

#include "Tool.h"

class SaveTraces: public Tool {


 public:

  SaveTraces();
  bool Initialise(std::string configfile,DataModel &data);
  bool Execute();
  bool Finalise();


 private:





};


#endif
