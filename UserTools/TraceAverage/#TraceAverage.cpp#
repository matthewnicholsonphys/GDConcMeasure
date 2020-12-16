#include "TraceAverage.h"

TraceAverage::TraceAverage():Tool(){}


bool TraceAverage::Initialise(std::string configfile, DataModel &data){

  if(configfile!="")  m_variables.Initialise(configfile);
  //m_variables.Print();

  m_data= &data;

  return true;
}


bool TraceAverage::Execute(){

  //if(m_data->state>PowerUP && m_data->state<Analyse){
  //std::cout<<"d1"<<std::endl;
  std::string measure;
  if(m_data->CStore.Get("Measure",measure) && measure == "Start"){

    //std::cout<<"d2"<<std::endl;
    
    std::vector<double> value;
    std::vector<double> error;   
    //std::cout<<"d2.1"<<std::endl;
    std::vector<double> wavelength=m_data->wavelength;
    //std::cout<<"d2.2 "<<m_data->measurment_time<<std::endl;
    tm mytm=to_tm(m_data->measurment_time);
    //std::cout<<"d2.3"<<std::endl;
    short year=mytm.tm_year + 1900;
    //std::cout<<"d2.4"<<std::endl;
    short month=mytm.tm_mon + 1;
    //std::cout<<"d2.5"<<std::endl;
    short day=mytm.tm_mday;
    //std::cout<<"d2.6"<<std::endl; 
    short hour=mytm.tm_hour; 
    //std::cout<<"d2.7"<<std::endl;
    short min=mytm.tm_min;
    //std::cout<<"d2.8"<<std::endl;
    std::string date=to_simple_string(m_data->measurment_time);
    //std::cout<<"d3"<<std::endl;

    std::string name="";
    m_data->CStore.Get("Trace",name);
    if(name=="") name="test";
    //std::stringstream state_name;
    //state_name<<m_data->state;
    //std::cout<<"d4"<<std::endl;
    TTree* tree=0;
    tree=m_data->GetTTree(name);
    if(tree==0){
      //std::cout<<"d5"<<std::endl;
      tree=new TTree(name.c_str(),name.c_str());
      m_data->AddTTree(name,tree);
      InitTTree(tree);
    }
    //std::cout<<"d6"<<std::endl;
    
    
    std::vector<double>* a=&value;
    std::vector<double>* b=&error;
    std::vector<double>* c=&wavelength;
    tree->SetBranchAddress("value", &a);
    tree->SetBranchAddress("error", &b);
    tree->SetBranchAddress("wavelength", &c);
    tree->SetBranchAddress("year", &year);
    tree->SetBranchAddress("month", &month);
    tree->SetBranchAddress("day", &day);
    tree->SetBranchAddress("hour", &hour);
    tree->SetBranchAddress("min", &min);
    //std::cout<<"d7 "<<m_data->wavelength.size()<<std::endl;
    
    for(int i=0;i<m_data->wavelength.size(); i++){
      double valsum=0;
      double errorsum=0;
      
      for(int trace=0;trace<m_data->traceCollect.size(); trace++){
	
	valsum+=m_data->traceCollect.at(trace).at(i)/((double)m_data->traceCollect.size());	
      }
      for(int trace=0;trace<m_data->traceCollect.size(); trace++){
	
        errorsum+=((m_data->traceCollect.at(trace).at(i)-valsum)*(m_data->traceCollect.at(trace).at(i)-valsum))/(((double)m_data->traceCollect.size())-1.0);
      }
      
      errorsum =sqrt(errorsum)/sqrt(m_data->traceCollect.size());
      value.push_back(valsum);
      error.push_back(errorsum);
    }
    //std::cout<<"d8"<<std::endl;
    tree->Fill();
    //std::cout<<"d9 "<<value.size()<<std::endl;
    TCanvas c1("c1","A Simple Graph with error bars",200,10,700,500);
    TGraphErrors gr(value.size(),&wavelength[0],&value[0],0,&error[0]);
    gr.SetTitle("TGraphErrors Example");
    //gr.SetMarkerColor(4);                                                                                                                   //gr.SetMarkerStyle(21);
    gr.Draw("AP");
    std::stringstream tmp;
    //    tmp<<it->first<<".png";
    tmp<<"/home/pi/WebServer/html/images/last.png";
    c1.SaveAs(tmp.str().c_str());
    //gr.Write(tmp.str().c_str(),TObject::kOverwrite);
    ////std::cout<<"a10 "<<std::endl
    //std::cout<<"d10"<<std::endl;

    m_data->CStore.Remove("Measure");
    
  }
  
  
  return true;
}


bool TraceAverage::Finalise(){
  
  return true;
}

void TraceAverage::InitTTree(TTree* tree){

  std::vector<double> value;
  std::vector<double> error;
  std::vector<double> wavelength;

  short year;
  short month;
  short day;
  short hour;
  short min;

  tree->Branch("value", &value);
  tree->Branch("error", &error);
  tree->Branch("wavelength", &wavelength);
  tree->Branch("year", &year, "year/S");
  tree->Branch("month", &month, "month/S");
  tree->Branch("day", &day, "day/S");
  tree->Branch("hour", &hour, "hour/S");
  tree->Branch("min", &min, "min/S");


}
