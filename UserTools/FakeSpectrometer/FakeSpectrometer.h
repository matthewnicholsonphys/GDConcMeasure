#ifndef FakeSpectrometer_H
#define FakeSpectrometer_H

#include <string>
#include <iostream>

#include "Tool.h"
#include "TRandom3.h"

class FakeSpectrometer: public Tool {


 public:

  FakeSpectrometer();
  bool Initialise(std::string configfile,DataModel &data);
  bool Execute();
  bool Finalise();

  bool GetDark();
  bool GetPure();
  bool GetData(double cc);
  double Pure(int i);
  double Absorb(int i, double cc);
  double Data(int i, double cc);
  double Dark(int i);
  double X(int i);

 private:

  TRandom3 *mt;

  double noise, pedestal;
  int nnn;

  double conc;

  std::vector<double> vx, vdark, vpure, vdata;



};


#endif
