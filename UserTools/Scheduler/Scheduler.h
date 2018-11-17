#ifndef Scheduler_H
#define Scheduler_H

#include <string>
#include <iostream>

#include "Tool.h"
#include "boost/date_time/gregorian/gregorian.hpp"
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/chrono.hpp>

class Scheduler: public Tool {


 public:

  Scheduler();
  bool Initialise(std::string configfile,DataModel &data);
  bool Execute();
  bool Finalise();


 private:


  boost::posix_time::ptime last;


};


#endif
