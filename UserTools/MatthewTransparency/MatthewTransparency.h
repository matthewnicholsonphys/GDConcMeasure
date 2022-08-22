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

  bool ok{true};
  bool toolactive{true};
  const int N{2068};

  std::vector<double> PopulateWavelength(TTree*);
  std::vector<double> RetrievePureValues(const int, const std::string) const;
  std::vector<double> RetrievePureValuesFromFile(const std::string&, const std::string&) const;
  void SaveToMonthlyFile(const Transparency&) const;
  std::vector<int> GetDateTimeVec() const;
  bool Ready() const;
  bool FileExists(std::string) const;
  void PlaceInDataModel(const Transparency&) const;
  std::vector<double> DarkSubtract(TTree*, TTree*) const;
  void MakeQuickPlot(const std::vector<double>, const std::vector<double>) const;
  bool HandleErrors();
};


#endif
