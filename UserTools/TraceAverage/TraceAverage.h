#ifndef TraceAverage_H
#define TraceAverage_H

#include <string>
#include <iostream>
#include <sstream>

#include "Tool.h"
#include "TTree.h"
#include "TCanvas.h"
#include "TGraphErrors.h"
#include "TColor.h"
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

  bool InitTTree(TTree* tree);
  
  bool livedraw=false;
  TCanvas* cspec=nullptr;
  TGraphErrors* ge=nullptr;
  double maxvalue=0;
  Color_t linecol = kRed;

  int verbosity=1;
  int v_error=0;
  int v_warning=1;
  int v_message=2;
  int v_debug=3;

};


#endif
