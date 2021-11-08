#include "BenPower.h"

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
  system(command.str().c_str());
  
  command.str("");
  command<<"echo \"out\" > /sys/class/gpio/gpio"<<m_powerpin<<"/direction";
  system(command.str().c_str());
   
  TurnOff();
   
  return true;
  
}

bool BenPower::Execute(){
  
  std::string Power="";
  
  if(m_data->CStore.Get("Power",Power) && Power!=power){
    
    if(Power=="ON") TurnOn();
    else if (Power=="OFF") TurnOff();
    
  }
  
  return true;
}


bool BenPower::Finalise(){
  
  TurnOff();
  
  return true;
}

void BenPower::TurnOn(){
  
  //write to GPIO
  Log("power on",v_message,verbosity);
  std::stringstream command;
  command<<"echo \"0\" > /sys/class/gpio/gpio"<<m_powerpin<<"/value";
  system(command.str().c_str());
  
  sleep(4);
  power ="ON";
}

void BenPower::TurnOff(){
  
  //write to GPIO
  Log("power off",v_message,verbosity);
  std::stringstream command;
  command<<"echo \"1\" > /sys/class/gpio/gpio"<<m_powerpin<<"/value";
  system(command.str().c_str());
  
  sleep(4);
  power="OFF";
  
}
