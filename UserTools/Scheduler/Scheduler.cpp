#include "Scheduler.h"

Scheduler::Scheduler():Tool(){}


bool Scheduler::Initialise(std::string configfile, DataModel &data){

  if(configfile!="")  m_variables.Initialise(configfile);
  //m_variables.Print();

  m_data= &data;

  m_data->mode=2;
  
  return true;
;
}


bool Scheduler::Execute(){
  //std::cout<<"mode="<<m_data->mode<<std::endl;
  if (m_data->mode==1){
    //    std::cout<<"mode1"<<std::endl;
    boost::posix_time::ptime current(boost::posix_time::second_clock::local_time());
    boost::posix_time::time_duration duration(current - last);
    boost::posix_time::time_duration period(0,0,10,0);
    //    std::cout<<duration<<std::endl;
    if (duration<period) usleep(500000);
    else{
      m_data->mode=2;
      std::cout<<"Start Up"<<std::endl;
    }
  }
  
  else if (m_data->mode==2){
    // std::cout<<"mode2"<<std::endl;
    last=boost::posix_time::second_clock::local_time();
    m_data->mode=3; std::cout<<"Pumping"<<std::endl;
    //  std::cout<<"t"<<std::endl;
  }
  
  else if (m_data->mode==3){
    boost::posix_time::ptime current(boost::posix_time::second_clock::local_time());
    boost::posix_time::time_duration duration(current - last);
    boost::posix_time::time_duration period(0,0,5,0);
    //    std::cout<<duration<<std::endl;
    if (duration<period) usleep(500000);
    else{
      m_data->mode=4;
      last=boost::posix_time::second_clock::local_time();
      std::cout<<"Setteling"<<std::endl;
    }
  }
  
    else if (m_data->mode==4){
      boost::posix_time::ptime current(boost::posix_time::second_clock::local_time());
      boost::posix_time::time_duration duration(current - last);
      boost::posix_time::time_duration period(0,0,5,0);
      //    std::cout<<duration<<std::endl;
      if (duration<period) usleep(500000);
      else{
	m_data->mode=5;
	std::cout<<"Measuring"<<std::endl;
      }
    }    
    
  
  return true;
}


bool Scheduler::Finalise(){

  return true;
}
