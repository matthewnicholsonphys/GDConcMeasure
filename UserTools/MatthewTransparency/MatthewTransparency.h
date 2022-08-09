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

  std::vector<double> RetrievePureValues(int, std::string) const;
  void SaveToMonthlyFile(Transparency) const;
  std::vector<int> GetDateTimeVec() const;
  bool Ready() const;
};


#endif
