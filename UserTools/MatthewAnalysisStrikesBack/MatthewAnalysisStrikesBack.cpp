#include "MatthewAnalysisStrikesBack.h"

#include "gad_utils.h"
#include <memory>

MatthewAnalysisStrikesBack::MatthewAnalysisStrikesBack():Tool(){}

bool MatthewAnalysisStrikesBack::Initialise(std::string configfile, DataModel &data){

  if(configfile!="")  m_variables.Initialise(configfile);
  //m_variables.Print();

  m_data = &data;

  try {
    const std::vector<std::string> list_of_leds = {"275_A", "275_B"};
    for (const auto& led_name : list_of_leds){
      LEDInfo led_info;

      const TGraph pure_ds = GetPure(led_name);
      const TGraph high_conc_ds = GetHighConc(led_name);
      
      std::unique_ptr<SimplePureFunc> simple_pure_func = std::make_unique<SimplePureFunc>(pure_ds);
      FunctionalFit simple_fit = FunctionalFit(simple_pure_func.get(), "simple"); 
      simple_fit.SetExampleGraph(pure_ds);

      simple_fit.PerformFitOnData(high_conc_ds);
      const TGraph simple_fit_result = simple_fit.GetGraph();
      const TGraph gd_abs_attenuation = CalculateGdAbs(simple_fit_result, high_conc_ds);
      const TGraph ratio_absorbance = PWRatio(simple_fit_result, high_conc_ds);

      std::unique_ptr<AbsFunc> abs_func = std::make_unique<AbsFunc>(ratio_absorbance);
      led_info.absorbtion_fit = FunctionalFit(abs_func.get(), "abs");
      led_info.absorbtion_fit.SetExampleGraph(pure_ds);

      // std::unique_ptr<CombinedGdPureFunc_DATA> combined_func = std::make_unique<CombinedGdPureFunc_DATA>(pure_ds, gd_abs_attenuation);      
      // led_info.combined_fit = FunctionalFit(combined_func.get(), "combined");
      // led_info.combined_fit.SetExampleGraph(pure_ds);
      
      led_info_map[led_name] = led_info;
    }
  }
  catch(const std::exception& e){
    std::cout << e.what() << "\n";
    return false;
  }

  return true;
}

bool MatthewAnalysisStrikesBack::Execute(){

  /*

    1. Get Current Data 
    2. Use Combined Fit on data
    3. Get Ratio absorption and trim
    3. Use abs fit on ratio abs 
    4. Get metric from fit
    5. Calculate the Gd prediction

   */
  
  
  return true;
}


bool MatthewAnalysisStrikesBack::Finalise(){

  return true;
}

TGraph MatthewAnalysisStrikesBack::GetPure(const std::string& led_name) {
  TGraph result;
  std::string pure_fname = "";
  int pure_offset = -1;
  bool ok = m_variables.Get("pure_fname", pure_fname) &&
    m_variables.Get("pure_offset", pure_offset);
  
  if (ok && pure_fname != "" && pure_offset != -1){
    result = GetDarkSubtractFromFile(pure_fname, led_name, pure_offset);
  }
  else {
    int  pureref_ver = -1;
    ok = m_variables.Get("pureref_ver", pureref_ver);
    if (ok && pureref_ver != -1){
      result = GetPureFromDB(pureref_ver, led_name);
    }
    else {
      throw std::runtime_error("MatthewAnalysisStrikesBack::GetPure - no pure specified!");
    }
  }
  return result;
}

TGraph MatthewAnalysisStrikesBack::GetHighConc(const std::string& led_name){
  TGraph result;
  std::string highconc_fname = "";
  int highconc_offset = -1;
  bool ok = m_variables.Get("highconc_fname", highconc_fname) &&
    m_variables.Get("highconc_offset", highconc_offset);
  
  if (ok && highconc_fname != "" && highconc_offset != -1){
    result = GetDarkSubtractFromFile(highconc_fname, led_name, highconc_offset);
  }
  else {
    int  highconcref_ver = -1;
    ok = m_variables.Get("highconcref_ver", highconcref_ver);
    if (ok && highconcref_ver != -1){
      result = GetHighConcFromDB(highconcref_ver, led_name);
    }
    else {
      throw std::runtime_error("MatthewAnalysisStrikesBack::GetHighconc - no highconc specified!");
    }
  }
  return result;
}


TGraph MatthewAnalysisStrikesBack::GetPureFromDB(const int& pureref_ver, const std::string& ledToAnalyse) const {
  std::string query_string = "SELECT values->'yvals' FROM data WHERE name='pure_curve'"
    " AND values->'yvals' IS NOT NULL AND ledname IS NOT NULL"
    " AND values->'version' IS NOT NULL"
    " AND ledname="+m_data->postgres.pqxx_quote(ledToAnalyse)+
    " AND values->'version'="+ m_data->postgres.pqxx_quote(pureref_ver);
  std::string pureref_json="";
  bool get_ok = m_data->postgres.ExecuteQuery(query_string, pureref_json);
  if(!get_ok || pureref_json==""){
    throw std::runtime_error("MatthewAnalysisStrikesBack::GetPureFromDB: Failed to obtain y values for pure reference for led: " +
			     ledToAnalyse+" and pure version: " + std::to_string(pureref_ver));
  }
  // the values string is a json array; i.e. '[val1, val2, val3...]'
  // strip the '[' and ']'
  pureref_json = pureref_json.substr(1,pureref_json.length()-2);
  // parse it
  std::stringstream ss(pureref_json);
  std::string tmp;
  std::vector<double> pureref_yvals;
  while(std::getline(ss,tmp,',')){
    char* endptr = &tmp[0];
    double nextval = strtod(tmp.c_str(),&endptr);
    if(endptr==&tmp[0]){
    throw std::runtime_error("MatthewAnalysisStrikesBack::GetPureFromDB: Failed to parse y values for pure reference for led: " +
			     ledToAnalyse+" and pure version: " + std::to_string(pureref_ver));
    }
    pureref_yvals.push_back(nextval);
  }
  if(pureref_yvals.size()==0){
    throw std::runtime_error("MatthewAnalysisStrikesBack::GetPureFromDB: Parse zero  y values for pure reference for led: " +
			     ledToAnalyse+" and pure version: " + std::to_string(pureref_ver));
  }
	
  // repeat for the x-values
  query_string = "SELECT values->'xvals' FROM data WHERE name='pure_curve'"
    " AND values->'xvals' IS NOT NULL AND ledname IS NOT NULL"
    " AND values->'version' IS NOT NULL"
    " AND ledname="+m_data->postgres.pqxx_quote(ledToAnalyse)+
    " AND values->'version'="+ m_data->postgres.pqxx_quote(pureref_ver);
  pureref_json="";
  get_ok = m_data->postgres.ExecuteQuery(query_string, pureref_json);
  if(!get_ok || pureref_json==""){
    throw std::runtime_error("MatthewAnalysisStrikesBack::GetPureFromDB: Failed to obtain x values for pure reference for led: " +
			     ledToAnalyse+" and pure version: " + std::to_string(pureref_ver));
  }
  // the values string is a json array; i.e. '[val1, val2, val3...]'
  // strip the '[' and ']'
  pureref_json = pureref_json.substr(1,pureref_json.length()-2);
  // parse it
  ss.clear();
  ss.str(pureref_json);
  std::vector<double> pureref_xvals;
  while(std::getline(ss,tmp,',')){
    char* endptr = &tmp[0];
    double nextval = strtod(tmp.c_str(),&endptr);
    if(endptr==&tmp[0]){
      throw std::runtime_error("MatthewAnalysisStrikesBack::GetPureFromDB: Failed to parse x values for pure reference for led: " +
			       ledToAnalyse+" and pure version: " + std::to_string(pureref_ver));
    }
    pureref_xvals.push_back(nextval);
  }
  if(pureref_xvals.size()==0){
    throw std::runtime_error("MatthewAnalysisStrikesBack::GetPureFromDB: Parsed zero x values for pure reference for led: " +
			     ledToAnalyse+" and pure version: " + std::to_string(pureref_ver));
  }
	
  TGraph dark_subtracted_pure = TGraph(pureref_xvals.size(), pureref_xvals.data(), pureref_yvals.data());
  std::string purename="g_pureref_"+ledToAnalyse;
  dark_subtracted_pure.SetName(purename.c_str());
  dark_subtracted_pure.SetTitle(purename.c_str());
  return dark_subtracted_pure;
}

TGraph MatthewAnalysisStrikesBack::GetHighConcFromDB(const int& highconcref_ver, const std::string& ledToAnalyse) const {
  std::string query_string = "SELECT values->'yvals' FROM data WHERE name='highconc_curve'"
    " AND values->'yvals' IS NOT NULL AND ledname IS NOT NULL"
    " AND values->'version' IS NOT NULL"
    " AND ledname="+m_data->postgres.pqxx_quote(ledToAnalyse)+
    " AND values->'version'="+ m_data->postgres.pqxx_quote(highconcref_ver);
  std::string highconcref_json="";
  bool get_ok = m_data->postgres.ExecuteQuery(query_string, highconcref_json);
  if(!get_ok || highconcref_json==""){
    throw std::runtime_error("MatthewAnalysisStrikesBack::GetHighconcFromDB: Failed to obtain y values for highconc reference for led: " +
			     ledToAnalyse+" and highconc version: " + std::to_string(highconcref_ver));
  }
  // the values string is a json array; i.e. '[val1, val2, val3...]'
  // strip the '[' and ']'
  highconcref_json = highconcref_json.substr(1,highconcref_json.length()-2);
  // parse it
  std::stringstream ss(highconcref_json);
  std::string tmp;
  std::vector<double> highconcref_yvals;
  while(std::getline(ss,tmp,',')){
    char* endptr = &tmp[0];
    double nextval = strtod(tmp.c_str(),&endptr);
    if(endptr==&tmp[0]){
    throw std::runtime_error("MatthewAnalysisStrikesBack::GetHighconcFromDB: Failed to parse y values for highconc reference for led: " +
			     ledToAnalyse+" and highconc version: " + std::to_string(highconcref_ver));
    }
    highconcref_yvals.push_back(nextval);
  }
  if(highconcref_yvals.size()==0){
    throw std::runtime_error("MatthewAnalysisStrikesBack::GetHighconcFromDB: Parse zero  y values for highconc reference for led: " +
			     ledToAnalyse+" and highconc version: " + std::to_string(highconcref_ver));
  }
	
  // repeat for the x-values
  query_string = "SELECT values->'xvals' FROM data WHERE name='highconc_curve'"
    " AND values->'xvals' IS NOT NULL AND ledname IS NOT NULL"
    " AND values->'version' IS NOT NULL"
    " AND ledname="+m_data->postgres.pqxx_quote(ledToAnalyse)+
    " AND values->'version'="+ m_data->postgres.pqxx_quote(highconcref_ver);
  highconcref_json="";
  get_ok = m_data->postgres.ExecuteQuery(query_string, highconcref_json);
  if(!get_ok || highconcref_json==""){
    throw std::runtime_error("MatthewAnalysisStrikesBack::GetHighConcFromDB: Failed to obtain x values for highconc reference for led: " +
			     ledToAnalyse+" and highconc version: " + std::to_string(highconcref_ver));
  }
  // the values string is a json array; i.e. '[val1, val2, val3...]'
  // strip the '[' and ']'
  highconcref_json = highconcref_json.substr(1,highconcref_json.length()-2);
  // parse it
  ss.clear();
  ss.str(highconcref_json);
  std::vector<double> highconcref_xvals;
  while(std::getline(ss,tmp,',')){
    char* endptr = &tmp[0];
    double nextval = strtod(tmp.c_str(),&endptr);
    if(endptr==&tmp[0]){
      throw std::runtime_error("MatthewAnalysisStrikesBack::GetHighconcFromDB: Failed to parse x values for highconc reference for led: " +
			       ledToAnalyse+" and highconc version: " + std::to_string(highconcref_ver));
    }
    highconcref_xvals.push_back(nextval);
  }
  if(highconcref_xvals.size()==0){
    throw std::runtime_error("MatthewAnalysisStrikesBack::GetHighconcFromDB: Parsed zero x values for highconc reference for led: " +
			     ledToAnalyse+" and highconc version: " + std::to_string(highconcref_ver));
  }
	
  TGraph dark_subtracted_highconc = TGraph(highconcref_xvals.size(), highconcref_xvals.data(), highconcref_yvals.data());
  std::string highconcname="g_highconcref_"+ledToAnalyse;
  dark_subtracted_highconc.SetName(highconcname.c_str());
  dark_subtracted_highconc.SetTitle(highconcname.c_str());
  return dark_subtracted_highconc;
}
