#ifndef MatthewTransparency_H
#define MatthewTransparency_H

#include <string>
#include <iostream>

#include "Tool.h"

struct Transparency;

class MatthewTransparency: public Tool {


 public:

  MatthewTransparency();
  bool Initialise(std::string configfile,DataModel &data);
  bool Execute();
  bool Finalise();

 private:
  
  std::vector<double> RetrievePureValues(const int, const std::string) const;
  std::vector<double> RetrievePureValuesFromFile(const std::string&, const std::string&) const;
  void SaveToMonthlyFile(const Transparency&) const;
  std::vector<int> GetDateTimeVec() const;
  bool Ready() const;
  void PlaceInDataModel(const Transparency&) const;

  void MakeQuickPlot(const std::vector<double>, const std::vector<double>) const;
};


#endif
