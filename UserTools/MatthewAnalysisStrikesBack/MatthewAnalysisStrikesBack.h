#ifndef MatthewAnalysisStrikesBack_H
#define MatthewAnalysisStrikesBack_H

#include "TGraph.h"

#include <map>
#include <string>
#include <iostream>
#include <memory>

#include "Tool.h"
#include "gad_utils.h"

struct LEDInfo {
  //  std::string name = "";
  std::string calib_fname = "";
  std::string calib_curve_name = "";
  int dark_offset = 0;
  //TGraph pure_ds;
  //TGraph high_conc_ds;
  //std::vector<double> initial_combined_fit_param_values;
  //std::vector<std::pair<double, double>> fit_param_ranges;#
  
  FunctionalFit combined_fit;
  FunctionalFit absorbtion_fit;
  TF1 calibration_curve;
  
};

class MatthewAnalysisStrikesBack: public Tool {

 public:

  MatthewAnalysisStrikesBack();
  bool Initialise(std::string configfile,DataModel &data);
  bool Execute();
  bool Finalise();

 private:

  std::map<std::string, LEDInfo> led_info_map;
  
  TGraph GetPure(const std::string& name);
  TGraph GetPureFromDB(const int&, const std::string&) const;
  TGraph GetHighConc(const std::string& name);
  TGraph GetHighConcFromDB(const int&, const std::string&) const;
};


#endif
