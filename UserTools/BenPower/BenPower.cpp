#include "BenPower.h"
#include "Algorithms.h"

BenPower::BenPower() : Tool(){}

bool BenPower::Initialise(std::string configfile, DataModel &data){
  
  if(configfile!="")
    m_variables.Initialise(configfile);
  //m_variables.Print();
  
  m_data= &data;
  
  m_variables.Get("verbosity",verbosity);
  
  if(!m_variables.Get("wake_pin",m_powerpin)){
    Log("no wake pin for powersupply set",v_error,verbosity);
    return false;
  }
  
  Log("power pin is "+std::to_string(m_powerpin),v_message,verbosity);
  
  std::stringstream command;
  command<<"echo \""<<m_powerpin<<"\" > /sys/class/gpio/export";
  std::string errmsg;
  int ok = SystemCall(command.str(), errmsg);
  if(ok!=0){
    Log("BenPower::Initialise "+errmsg,0,0);
    return false;
  }
  
  command.str("");
  command<<"echo \"out\" > /sys/class/gpio/gpio"<<m_powerpin<<"/direction";
  ok = SystemCall(command.str(), errmsg);
  if(ok!=0){
    Log("BenPower::Initialise "+errmsg,0,0);
    return false;
  }
  
  ok = TurnOff();
  
  return ok;
  
}

bool BenPower::Execute(){
  
  std::string Power="";
  int ok = true;
  
  if(m_data->CStore.Get("Power",Power) && Power!=power){
    
    if(Power=="ON") ok = TurnOn();
    else if (Power=="OFF") ok = TurnOff();
    
  }
  
  return ok;
}


bool BenPower::Finalise(){
  
  bool ok = TurnOff();
  
  return ok;
}

bool BenPower::TurnOn(){
  
  //write to GPIO
  Log("power on",v_message,verbosity);
  std::stringstream command;
  command<<"echo \"0\" > /sys/class/gpio/gpio"<<m_powerpin<<"/value";
  
  std::string errmsg;
  int retval=SystemCall(command.str(), errmsg);
  if(retval!=0){
    Log("BenPower::TurnOn "+errmsg,0,0);
    return false;
  }
  
  sleep(4);
  power ="ON";
  
  return true;
  
}

bool BenPower::TurnOff(){
  
  //write to GPIO
  Log("power off",v_message,verbosity);
  std::stringstream command;
  command<<"echo \"1\" > /sys/class/gpio/gpio"<<m_powerpin<<"/value";
  
  std::string errmsg;
  int retval = SystemCall(command.str(),errmsg);
  if(retval!=0){
    Log("BenPower::TurnOff "+errmsg,0,0);
    return false;
  }
  
  sleep(4);
  power="OFF";
  
  return true;
  
}
