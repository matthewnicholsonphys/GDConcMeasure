#include "BenPower.h"

BenPower::BenPower() : Tool(){}

bool BenPower::Initialise(std::string configfile, DataModel &data){
  
  if(configfile!="")
    m_variables.Initialise(configfile);
  //m_variables.Print();

  m_data= &data;
  
  if(!m_variables.Get("wake_pin",m_powerpin)){

    std::cout<<"no wake pin for powersupply set"<<std::endl;
    return false;
  }

  std::cout<<"power pin is "<<m_powerpin<<std::endl;
  
  std::stringstream command;
  command<<"echo \""<<m_powerpin<<"\" > /sys/class/gpio/export";
  system(command.str().c_str());
  //std::cout<<"d1 "<<command.str() <<std::endl;
  
  command.str("");
  command<<"echo \"out\" > /sys/class/gpio/gpio"<<m_powerpin<<"/direction";
  system(command.str().c_str());
  //std::cout<<"d2"<<std::endl;
   /*
   command.str("");

   command<<"echo \""<<"17"<<"\" > /sys/class/gpio/export";
  system(command.str().c_str());
  std::cout<<"d1 "<<command.str() <<std::endl;

  command.str("");
  command<<"echo \"out\" > /sys/class/gpio/gpio"<<"17"<<"/direction";
  system(command.str().c_str());
   std::cout<<"d2"<<std::endl;
   */
   
  TurnOff();
  //std::cout<<"d3"<<std::endl;
   
  return true;

}

bool BenPower::Execute(){

  std::string Power="";
  
  if(m_data->CStore.Get("Power",Power) && Power!=power){

    if(Power=="ON") TurnOn();
    else if (Power=="OFF") TurnOff();
    
  }
  
  //  if(m_data->state==PowerUP) TurnOn();
  //else if(m_data->state==Sleep) TurnOff();
  
  return true;
}


bool BenPower::Finalise(){
  
  TurnOff();

  return true;
}

void BenPower::TurnOn(){
  
  //write to GPIO
  std::cout<<"power on"<<std::endl;  
  std::stringstream command;
  command<<"echo \"0\" > /sys/class/gpio/gpio"<<m_powerpin<<"/value";
  system(command.str().c_str());
  //sleep(2);
  //  command.str("");
  //command<<"echo \"1\" > /sys/class/gpio/gpio"<<"17"<<"/value";
  //system(command.str().c_str());
  
  sleep(4);
  power ="ON";  
}

void BenPower::TurnOff(){
  
  //write to GPIO
  std::cout<<"power off"<<std::endl;
  std::stringstream command;
  command<<"echo \"1\" > /sys/class/gpio/gpio"<<m_powerpin<<"/value";
  system(command.str().c_str());
  //command.str("");
  //command<<"echo \"0\" > /sys/class/gpio/gpio"<<"17"<<"/value";
  //system(command.str().c_str());
  
  sleep(4);
  power="OFF";

}
