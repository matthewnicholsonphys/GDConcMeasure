#include "MatthewTransparency.h"

#include <sys/stat.h>

#include <memory>
#include <chrono>
#include <time.h>
#include <vector>
#include <string>
#include <sstream>

#include "TTree.h"
#include "TFile.h"

struct Transparency {
  std::vector<double> values;
  std::vector<int> date_time;
};

MatthewTransparency::MatthewTransparency():Tool(){}


bool MatthewTransparency::Initialise(std::string configfile, DataModel &data){

  if(configfile!="")  m_variables.Initialise(configfile);
  //m_variables.Print();
  m_data= &data;

  return true;
}


bool MatthewTransparency::Execute(){

  if (Ready()){
  
    int pure_ref_ver{5}; // ???

    std::vector<double> transparency_values;
  
    const int N{2068};
    std::vector<double> pure_dark_sub;
    std::vector<double> dark_sub_sum(N), pure_sum(N);
    std::vector<double> *led_values, *dark_values, *wavelengths;
  
    for (auto [tree_name, tree_ptr] : m_data->m_trees){
      if (tree_name != "Dark"){
	pure_dark_sub = RetrievePureValues(pure_ref_ver, tree_name);
	tree_ptr->SetBranchAddress("value", &led_values);
	tree_ptr->SetBranchAddress("wavelength", &wavelengths);
      }
      else {      
	tree_ptr->SetBranchAddress("value", &dark_values);
      }
      if (led_values && dark_values){
	for (auto i = 0; i < N; ++i){
	  dark_sub_sum.at(i) += led_values->at(i) - dark_values->at(i);
	  pure_sum.at(i) += pure_dark_sub.at(i);
	}
	led_values = nullptr;
	dark_values = nullptr;
      }
    }

    for (auto i = 0; i < N; ++i){
      transparency_values.push_back(dark_sub_sum.at(i) / pure_sum.at(i));
    }

    Transparency t{transparency_values, GetDateTimeVec()};
    SaveToMonthlyFile(t);
  }  
  return true;
}


bool MatthewTransparency::Finalise(){

  return true;
}

/*
1. retrieve pure trace
2. calculate transparency for current  measurment cycle - with errors
3. save trace values (vector) to datamodel for ploting
4. plot trace to website
5. save raw transparency values to file in ttree, changing file once per month.
*/


std::vector<double> MatthewTransparency::RetrievePureValues(int pureref_ver, std::string led) const {
  
  std::string query_string{"SELECT values->'yvals' FROM data WHERE name='pure_curve'"
			   " AND values->'yvals' IS NOT NULL AND values->'led' IS NOT NULL"
			   " AND values->'version' IS NOT NULL"
			   " AND values->>'led'="+m_data->postgres.pqxx_quote(led)+
			   " AND values->'version'="+ m_data->postgres.pqxx_quote(pureref_ver)};
  std::string pureref_json{""};

  m_data->postgres.ExecuteQuery(query_string, pureref_json);
  pureref_json = pureref_json.substr(1,pureref_json.length()-2);

  std::stringstream ss(pureref_json);
  std::string tmp;
  std::vector<double> pureref_yvals;
  while(std::getline(ss,tmp,',')){
    char* endptr = &tmp[0];
    pureref_yvals.push_back(strtod(tmp.c_str(), &endptr));
  }
  return pureref_yvals;
  
}

void MatthewTransparency::SaveToMonthlyFile(Transparency t) const {
  
  const std::vector<double>* v_ptr = &(t.values);
  const std::vector<int>* t_ptr = &(t.date_time);

  const std::string transp_str{"transparency"}, val_str{"values"}, dt_str{"date_time"};
  const std::string fname{transp_str + "_" + t.date_time.at(0) + "_" + t.date_time.at(1) + ".root"};

  struct stat s;
  if (stat(fname.c_str(), &s)==0) {
    std::unique_ptr<TFile> file(TFile::Open(fname.c_str(), "UPDATE"));
    TTree* tree = (TTree*) file->Get(transp_str.c_str());
    tree->SetBranchAddress(val_str.c_str(), &v_ptr);
    tree->SetBranchAddress(dt_str.c_str(), &t_ptr);
    tree->Fill();
    file->Write("*", TObject::kOverwrite);
  }
  else {
    std::unique_ptr<TFile> file(TFile::Open(fname.c_str(), "RECREATE"));
    std::unique_ptr<TTree> tree(new TTree(transp_str.c_str(), transp_str.c_str()));
    tree->Branch(val_str.c_str(), &v_ptr);
    tree->Branch(dt_str.c_str(), &t_ptr);
    tree->Fill();
    file->Write();
  }
}


std::vector<int> MatthewTransparency::GetDateTimeVec() const{
  const auto now = std::chrono::system_clock::now();
  const std::time_t t = std::chrono::system_clock::to_time_t(now);
  tm local_tm = *localtime(&t);
  
  return {local_tm.tm_year+1900, local_tm.tm_mon+1, local_tm.tm_mday+1,
	  local_tm.tm_hour, local_tm.tm_min, local_tm.tm_sec};
}

bool MatthewTransparency::Ready() const {
  bool ready{false};
  std::string transparency;
  bool success = m_data->CStore.Get("Transparency", transparency);
  if (success && transparency == "Transparency"){
    m_data->CStore.Remove("Transparency");
    transparency.clear();
    ready = true;
  }
  return ready;
}

//TODO: plot to website
