#include "Calibration.h"

Calibration::Calibration():Tool(){}


bool Calibration::Initialise(std::string configfile, DataModel &data){

  if(configfile!="")  m_variables.Initialise(configfile);
  //m_variables.Print();

  m_data= &data;

  return true;
}


bool Calibration::Execute(){

  return true;
}


bool Calibration::Finalise(){

  return true;
}
