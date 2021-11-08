#include "Valve.h"

Valve::Valve():Tool(){}


bool Valve::Initialise(std::string configfile, DataModel &data){
  
  if(configfile!="")  m_variables.Initialise(configfile);
  //m_variables.Print();
  
  m_data= &data;
  
  m_variables.Get("verbosity",verbosity);
  
  if (!m_variables.Get("valve_pin",m_valve_pin)){
    
    Log("Valve pin not set",v_error,verbosity);
    return false;
  }
  
  std::stringstream command;
  command<<"echo \""<<m_valve_pin<<"\" > /sys/class/gpio/export";
  system(command.str().c_str());
  
  command.str("");
  command<<"echo \"out\" > /sys/class/gpio/gpio"<<m_valve_pin<<"/direction";
  system(command.str().c_str());
  
  ValveClose();
  
  return true;
}


bool Valve::Execute(){
  
  std::string Valve="";
  
  if(m_data->CStore.Get("Valve",Valve) && Valve!=valve){
    if(Valve=="OPEN"){
      ValveOpen();
    }
    if(Valve=="CLOSE"){
      ValveClose();
    }
      
  }
  
  return true;
}


bool Valve::Finalise(){
  
  ValveClose();
  
  return true;
}


void Valve::ValveOpen(){
  
  Log("valve open",v_message,verbosity);
  std::stringstream command;
  command<<"echo \"1\" > /sys/class/gpio/gpio"<<m_valve_pin<<"/value";
  system(command.str().c_str());
  valve="OPEN";
}

void Valve::ValveClose(){
  
  Log("valve close",v_message,verbosity);
  std::stringstream command;
  command<<"echo \"0\" > /sys/class/gpio/gpio"<<m_valve_pin<<"/value";
  system(command.str().c_str());
  valve="CLOSE";
}
