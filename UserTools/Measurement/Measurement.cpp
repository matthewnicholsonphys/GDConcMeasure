#include "Measurement.h"

Measurement::Measurement():Tool(){}


bool Measurement::Initialise(std::string configfile, DataModel &data){

  if(configfile!="")  m_variables.Initialise(configfile);
  //m_variables.Print();

  m_data= &data;

  return true;
}


bool Measurement::Execute(){

  return true;
}


bool Measurement::Finalise(){

  return true;
}
