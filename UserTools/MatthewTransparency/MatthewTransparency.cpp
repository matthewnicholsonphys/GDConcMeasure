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
  if (true /*Ready()*/){

    const std::string pure_file_name{"~/GDConcMeasure/data/2022/07/00117_12July22Calib_00000.root"};
    
    const int pure_ref_ver{5}; //when DB support is ready
  
    std::vector<double> pure_dark_sub;
    std::vector<double> dark_sub_sum(N), pure_sum(N), wavelengths(N);

    TTree *dark_tree_ptr = nullptr, *led_tree_ptr = nullptr;
    for (const auto& [tree_name, tree_ptr] : m_data->m_trees){
      if (wavelengths.back() == 0){PopulateWavelength(tree_ptr);} 
      if (tree_name != "Dark"){
	led_tree_ptr = tree_ptr;
	//pure_dark_sub = RetrievePureValues(pure_ref_ver, tree_name); //add this back in when database support is ready // TODO: only calculate once!
	pure_dark_sub = RetrievePureValuesFromFile(pure_file_name, tree_name);
      }
      else {
	dark_tree_ptr = tree_ptr;
      }
      if (led_tree_ptr && dark_tree_ptr){
	std::vector<double> dark_sub = DarkSubtract(led_tree_ptr, dark_tree_ptr);
	for (auto i = 0; i < N; ++i){
	  pure_sum.at(i) += pure_dark_sub.at(i);
	  dark_sub_sum.at(i) += dark_sub.at(i);
	}
	led_tree_ptr = nullptr;
	dark_tree_ptr = nullptr;
      }
    }

    const std::vector<double> transparency_values = [=](){
						      std::vector<double> result;
						      for (auto i = 0; i < N; ++i){
							result.push_back(dark_sub_sum.at(i) / pure_sum.at(i));
						      }
						      return result;
						    }();

    const Transparency t{transparency_values, GetDateTimeVec()};
    //SaveToMonthlyFile(t);
    //PlaceInDataModel(t);

    MakeQuickPlot(t.values, wavelengths);
  }
      
  return true;
}
 
std::vector<double> MatthewTransparency::PopulateWavelength(TTree* tree){
  // Takes a TTree* and returns an N sized vector with wavelength values, this is split from usual TTree reading since we only need to calculate wavelength vector once.
  
    std::vector<double>* w_ptr = nullptr;
  tree->SetBranchAddress("wavelength", &w_ptr);

  if (w_ptr == nullptr){
    std::cout << "Failed to set branch address to retrieve wavelength" << std::endl;
  }
  else if (tree->GetEntriesFast() == 0){
    std::cout << "No entries in wavelength tree" << std::endl;
  }

  const bool success = tree->GetEntry(0);
  const auto wav = [=](){
		     std::vector<double> result;
		     for (auto i = 0; i < N; ++i){result.push_back(w_ptr->at(i));}
		     return result;}();
  return wav;
}
 
std::vector<double> MatthewTransparency::DarkSubtract(TTree* led, TTree* dark) const {
  // Takes two TTree* and returns an N sized vector containing dark subtracted intensity values of the LED that TTree* led corresponds to. 
  std::vector<double> *led_v = nullptr, *dark_v = nullptr;

  led->SetBranchAddress("value", &led_v);
  dark->SetBranchAddress("value", &dark_v);

  if (led_v == nullptr || dark_v == nullptr){
    std::cout << "Failed to set branch address in dark subtract calculation" << std::endl;
  }
  else if (led->GetEntriesFast() == 0 || dark->GetEntriesFast() == 0){
    std::cout << "No entries in dark or led tree" << std::endl;
  }
  led->GetEntry(0);
  dark->GetEntry(0);
  
  const auto dark_sub = [=](){
			       std::vector<double> result;
			       for (auto i = 0; i < N; ++i){
				 result.push_back(led_v->at(i) - dark_v->at(i));
			       }
			       return result;
			     }();
  
  return dark_sub;
}

bool MatthewTransparency::Finalise(){

  return true;
}

std::vector<double> MatthewTransparency::RetrievePureValues(const int pureref_ver, const std::string led) const {
  // Retrieve dark subtracted pure values from database for a given LED
  const  std::string query_string{"SELECT values->'yvals' FROM data WHERE name='pure_curve'"
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

std::vector<double> MatthewTransparency::RetrievePureValuesFromFile(const std::string& fname, const std::string& led) const {
  // Until database integration is finished we'll just get the pure values from the first file in the calibration run. 
  std::unique_ptr<TFile> file(TFile::Open(fname.c_str()));
    TTree *led_tree = nullptr, *dark_tree = nullptr;
    
    led_tree = (TTree*) file->Get(led.c_str());
    dark_tree = (TTree*) file->Get("Dark");

    if (led_tree == nullptr || dark_tree == nullptr){
      std::cout << "couldn't get trees" << std::endl;
    }
    
    const std::vector<double> dark_sub = DarkSubtract(led_tree, dark_tree);
    return dark_sub;
}

void MatthewTransparency::SaveToMonthlyFile(const Transparency& t) const {
  // Transparency values will be stored in a file that is newly generated once per month, 
  const std::vector<double>* v_ptr = &(t.values);
  const std::vector<int>* t_ptr = &(t.date_time);

  const std::string transp_str{"transparency"}, val_str{"values"}, dt_str{"date_time"};
  const std::string fname{transp_str + "_" + t.date_time.at(0) + "_" + t.date_time.at(1) + ".root"};

  if (FileExists(fname)){
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

bool MatthewTransparency::FileExists(std::string f) const {
  struct stat s;
  if (stat(f.c_str(), &s) == 0){
    return true;
  } else {
    return false;
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
  std::string transparency{""};
  bool success = m_data->CStore.Get("Transparency", transparency);
  if (success && transparency == "Transparency"){
    m_data->CStore.Remove("Transparency");
    transparency.clear();
    ready = true;
  }
  return ready;
}

void MatthewTransparency::PlaceInDataModel(const Transparency& t) const {
  m_data->transparency_values = t.values;
  m_data->transparency_date_time = t.date_time;
}

void MatthewTransparency::MakeQuickPlot(const std::vector<double> v, const std::vector<double> w) const {
  TGraph g(v.size(), w.data(), w.data());
  TCanvas c1("c1", "c1", 2000, 2000);
  g.Draw();
  c1.SaveAs("/home/pi/GDConcMeasure/trans_plot.root");
  
}
