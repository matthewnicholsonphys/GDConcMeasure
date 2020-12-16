#ifndef TraceAverage_H
#define TraceAverage_H

#include <string>
#include <iostream>
#include <sstream>

#include "Tool.h"
#include "TTree.h"
#include "TCanvas.h"
#include "TGraphErrors.h"
#include <math.h>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/date_time/posix_time/posix_time_io.hpp>

class TraceAverage: public Tool {


 public:

  TraceAverage();
  bool Initialise(std::string configfile,DataModel &data);
  bool Execute();
  bool Finalise();


 private:

  void InitTTree(TTree* tree);



};


#endif
