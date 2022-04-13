#include "TraceAverage.h"

TraceAverage::TraceAverage():Tool(){}


bool TraceAverage::Initialise(std::string configfile, DataModel &data){
  
  if(configfile!="")  m_variables.Initialise(configfile);
  //m_variables.Print();
  
  m_data= &data;
  
  return true;
}


bool TraceAverage::Execute(){
  
  std::string measure;
  if(m_data->CStore.Get("Measure",measure) && measure == "Start"){
    
    std::vector<double> value;
    std::vector<double> error;
    std::vector<double> wavelength=m_data->wavelength;
    
    // convert timestamp to string
    tm mytm=to_tm(m_data->measurment_time);
    short year=mytm.tm_year + 1900;
    short month=mytm.tm_mon + 1;
    short day=mytm.tm_mday;
    short hour=mytm.tm_hour;
    short min=mytm.tm_min;
    short sec=mytm.tm_sec;
    std::string date=to_simple_string(m_data->measurment_time);
    
    // get trace name - we'll use this for the TTree name
    std::string name="";
    m_data->CStore.Get("Trace",name);
    if(name=="") name="test";
    
    // see if we have this TTree in the datamodel, or build one if not
    TTree* tree=0;
    tree=m_data->GetTTree(name);
    if(tree==0){
      tree=new TTree(name.c_str(),name.c_str());
      m_data->AddTTree(name,tree);
      bool ok = InitTTree(tree);
      // don't return, is it better that we at least make a graph for the website...?
    }
    
    // make local variables to hold measurement data
    std::vector<double>* a=&value;
    std::vector<double>* b=&error;
    std::vector<double>* c=&wavelength;
    bool err=false;
    // setup tree addresses
    err |= (tree->SetBranchAddress("value", &a) < 0);
    err |= (tree->SetBranchAddress("error", &b) < 0);
    err |= (tree->SetBranchAddress("wavelength", &c) < 0);
    err |= (tree->SetBranchAddress("year", &year) < 0);
    err |= (tree->SetBranchAddress("month", &month) < 0);
    err |= (tree->SetBranchAddress("day", &day) < 0);
    err |= (tree->SetBranchAddress("hour", &hour) < 0);
    err |= (tree->SetBranchAddress("min", &min) < 0);
    err |= (tree->SetBranchAddress("sec", &sec) < 0);
    if(err){
      Log("TraceAverage::Execute error setting branch addresses",0,0);
      // don't return, is it better that we at least make a graph for the website...?
    }
    
    // retrieve data from datamodel, averaging over spectrometer acquisitions
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
    
    // fill TTree
    int nbytes = tree->Fill();
    if(nbytes<=0){
      Log("TraceAverage::Execute error calling TTree::Fill, returned "+std::to_string(nbytes),0,0);
      // don't return, is it better that we at least make a graph for the website...?
    }
    
    // make a TGraph for the website
    TCanvas c1("c1","A Simple Graph with error bars",200,10,700,500);
    TGraphErrors gr(value.size(),&wavelength[0],&value[0],0,&error[0]);
    gr.SetTitle("TGraphErrors Example");
    //gr.SetMarkerColor(4);
    //gr.SetMarkerStyle(21);
    gr.Draw("AP");
    c1.SaveAs("/home/pi/WebServer/html/images/last.png");
    //gr.Write(tmp.str().c_str(),TObject::kOverwrite);
    
    m_data->CStore.Remove("Measure");
    
  }
  
  
  return true;
}


bool TraceAverage::Finalise(){
  
  return true;
}

bool TraceAverage::InitTTree(TTree* tree){
  
  std::vector<double> value;
  std::vector<double> error;
  std::vector<double> wavelength;
  
  short year;
  short month;
  short day;
  short hour;
  short min;
  short sec;
  
  int bptr = 1;
  bptr &= intptr_t(tree->Branch("value", &value));
  bptr &= intptr_t(tree->Branch("error", &error));
  bptr &= intptr_t(tree->Branch("wavelength", &wavelength));
  bptr &= intptr_t(tree->Branch("year", &year, "year/S"));
  bptr &= intptr_t(tree->Branch("month", &month, "month/S"));
  bptr &= intptr_t(tree->Branch("day", &day, "day/S"));
  bptr &= intptr_t(tree->Branch("hour", &hour, "hour/S"));
  bptr &= intptr_t(tree->Branch("min", &min, "min/S"));
  bptr &= intptr_t(tree->Branch("sec", &sec, "sec/S"));
  
  if(!bptr){
    Log("TraceAverage::InitTTree error making TTree branches!",0,0);
    return false;
  }
  return true;
  
}
