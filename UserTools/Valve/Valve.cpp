#include "Valve.h"
#include "Algorithms.h"

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
  std::string errmsg;
  int ok = SystemCall(command.str(), errmsg);
  if(ok!=0){
    Log("Valve::Initialise "+errmsg,0,0);
    return false;
  }
  
  command.str("");
  command<<"echo \"out\" > /sys/class/gpio/gpio"<<m_valve_pin<<"/direction";
  ok = SystemCall(command.str(),errmsg);
  if(ok!=0){
    Log("Valve::Initialise "+errmsg,0,0);
    return false;
  }
  
  ok = ValveClose();
  if(not ok) return ok;
  
  return true;
}


bool Valve::Execute(){
  
  std::string Valve="";
  bool ok = true;
  
  if(m_data->CStore.Get("Valve",Valve) && Valve!=valve){
    if(Valve=="OPEN"){
      ok = ValveOpen();
    }
    if(Valve=="CLOSE"){
      ok = ValveClose();
    }
  }
  
  return ok;
}


bool Valve::Finalise(){
  
  bool ok = ValveClose();
  
  return ok;
}


bool Valve::ValveOpen(){
  
  Log("valve open",v_message,verbosity);
  std::stringstream command;
  command<<"echo \"1\" > /sys/class/gpio/gpio"<<m_valve_pin<<"/value";
  std::string errmsg;
  int ok = SystemCall(command.str(), errmsg);
  if(ok!=0){
    Log("Valve::ValveOpen "+errmsg,0,0);
    return false;
  }
  valve="OPEN";
  return true;
}

bool Valve::ValveClose(){
  
  Log("valve close",v_message,verbosity);
  std::stringstream command;
  command<<"echo \"0\" > /sys/class/gpio/gpio"<<m_valve_pin<<"/value";
  std::string errmsg;
  int ok = SystemCall(command.str(), errmsg);
  if(ok!=0){
    Log("Valve::ValveClose "+errmsg,0,0);
    return false;
  }
  valve="CLOSE";
  return true;
}
