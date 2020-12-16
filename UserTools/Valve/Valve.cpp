#include "Valve.h"

Valve::Valve():Tool(){}


bool Valve::Initialise(std::string configfile, DataModel &data){

  if(configfile!="")  m_variables.Initialise(configfile);
  //m_variables.Print();

  m_data= &data;

  if (!m_variables.Get("valve_pin",m_valve_pin)){

    std::cout<<"Valve pin not set"<<std::endl;
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

  //  std::cout<<std::endl<<std::endl<<std::endl<<"valve="<<valve<<std::endl;
  //std::cout<<"Valve="<<Valve<<std::endl;
  //m_data->CStore.Print();
  //std::cout<<"if eval="<<(valve!=Valve)<<std::endl;
  
  if(m_data->CStore.Get("Valve",Valve) && Valve!=valve){
    //std::cout<<"Valve in ="<<Valve<<std::endl;
    //std::cout<<"Valve close?? ="<<(Valve=="CLOSE")<<std::endl;
    if(Valve=="OPEN"){
      //std::cout<<"OPEN!!!!!!!!!!!"<<std::endl;
      ValveOpen();
    }
    //std::cout<<"Valve close?? ="<<(Valve=="CLOSE")<<std::endl;
    if(Valve=="CLOSE"){
      //std::cout<<"CLOSE!!!!!!!!!!!"<<std::endl;
      ValveClose();
    }
      
  }
  //sleep(2);
  //if (m_data->state==ReplaceWater) ValveOpen();
  //else ValveClose();
  
  return true;
}


bool Valve::Finalise(){

  ValveClose();
  
  return true;
}


void Valve::ValveOpen(){

  std::cout<<"valve open"<<std::endl;
  std::stringstream command;
  command<<"echo \"1\" > /sys/class/gpio/gpio"<<m_valve_pin<<"/value";
  system(command.str().c_str());
  valve="OPEN";
}

void Valve::ValveClose(){

  std::cout<<"vavle close"<<std::endl;
  std::stringstream command;
  command<<"echo \"0\" > /sys/class/gpio/gpio"<<m_valve_pin<<"/value";
  system(command.str().c_str());
  valve="CLOSE";
}
