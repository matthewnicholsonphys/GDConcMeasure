#include "Spectrometer.h"

Spectrometer::Spectrometer():Tool(){}


bool Spectrometer::Initialise(std::string configfile, DataModel &data){

  if(configfile!="")  m_variables.Initialise(configfile);
  //m_variables.Print();

  m_data= &data;

  return true;
}


bool Spectrometer::Execute(){

  return true;
}


bool Spectrometer::Finalise(){

  return true;
}
