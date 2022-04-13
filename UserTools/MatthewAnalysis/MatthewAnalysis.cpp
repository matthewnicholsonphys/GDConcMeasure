#include "MatthewAnalysis.h"

#include <boost/algorithm/string.hpp>

#include "TCanvas.h"
#include "TGraphErrors.h"
#include "TTree.h"
#include "TF1.h"

#include <map>
MatthewAnalysis::MatthewAnalysis():Tool(){}

bool MatthewAnalysis::Initialise(std::string configfile, DataModel &data){

  if(configfile!="")  m_variables.Initialise(configfile);
  //m_variables.Print();
  m_data= &data;

  bool success = (RetrieveDarkSubPure() && RetrieveCalibrationCurve());
  return success;
}

bool MatthewAnalysis::Execute(){
  
  // check if we have an Analyse flag in the CStore indicating intensity data
  // is available for fitting
  if (ReadyToAnalyse()){
    
    // get the TTrees containing LED-on and LED-off traces
    std::pair<TTree*, TTree*> dark_and_led_trees = FindDarkAndLEDTrees();
    TTree* dark_tree = dark_and_led_trees.first, *led_tree = dark_and_led_trees.second;
    if(dark_tree==nullptr || led_tree==nullptr){
      std::stringstream tmp;
      tmp << "MatthewAnalysis::FindDarkAndLEDTrees could not find signal ("<<led_tree
          << ") and/or dark ("<<dark_tree<<") TTree in DataModel";
      Log(tmp.str(),0,0);
      return false;
    }
    
    // pull LED-on and LED-off traces from TTrees and produce TGraph of dark-subtracted trace
    TGraphErrors dark_subtracted_gd = PerformDarkSubtraction(led_tree, dark_tree);
    if(dark_subtracted_gd.GetN()==0){
      return false;  // failed to get data from TTrees
    }
    
    // Remove points in the absorption region 270-280nm TODO make this range configurable
    TGraphErrors dark_sub_gd_w_abs_region_removed = RemoveRegion(dark_subtracted_gd);

    // taking the reference pure water trace as a function, fit this function, scaled
    // and with a linear background added, to the measured trace with the absorption region removed.
    // The fit paramters represent the scaling and linear background added.
    double* fitting_parameters = FunctionalFit(dark_sub_gd_w_abs_region_removed);
    if(fitting_parameters==nullptr){
      return false;   // fit failed
    }
    double* fitting_errors; // TODO can be obtained from TFitResultPtr in FunctionalFit for example
    
    // scale pure water trace by fit function parameters to obtain a version of the pure water
    // spectrum that should match the LED-on spectrum in the lobes of the LED peak
    TGraphErrors dark_subtracted_pure = *(m_data->dark_sub_pure);
    TGraphErrors pure_scaled = Scale(dark_subtracted_pure, fitting_parameters, fitting_errors);
    
    // use the difference between the scaled pure water trace and the current LED-on trace
    // in the gd absorption-peak region to determine the amount of absorbance due to Gd.
    // absorbance is defined as the log_10(received/transmitted light)
    TGraphErrors absorbance = CalculateAbsorbance(pure_scaled, dark_subtracted_gd);
    
    // fit the two main absorption peaks at 273 and 276 nm with gaussians,
    // then take difference in their amplitudes. As with the amplitudes of each peak,
    // this should be related to concentration.
    std::vector<double> peaksdiff_w_error = FindPeakDifference(absorbance);
    //std::vector<double> peaksdiff_w_error = {0.02 * m_data->concs_and_peakdiffs.size() + 0.005, 0};
    if(peaksdiff_w_error==std::vector<double>{0,0}){
      return false;   // failure fitting gaussians to the two absorbance peaks
    }
    
    // use calibration curve to map difference in height of the two main absorbance peaks to gd concentration
    std::vector<double> concentration_w_error = CalculateConcentration(peaksdiff_w_error);
    m_data->concs_and_peakdiffs.push_back(std::make_pair(concentration_w_error.at(0), peaksdiff_w_error.at(0)));
    
    // Inform downstream tools that a new measurement is available
    // maybe we could use the value to indicate if the data is good?
    m_data->CStore.Set("NewMeasurement",true);
    
    // debug prints
    std::cout << "Peak difference from " << "ledName" << " is: " << peaksdiff_w_error.at(0) << " +/- " << peaksdiff_w_error.at(1) << '\n';
    std::cout << "Concentration from " << "ledName" << " is: " << concentration_w_error.at(0) << " +/- " << concentration_w_error.at(1) << '\n';
    
  } else {
    // else no data to Analyse
    m_data->CStore.Remove("NewMeasurement");
  }
  
  return true;
}

bool MatthewAnalysis::Finalise(){
  
  delete m_data->dark_sub_pure;
  delete m_data->pure_fct;
  delete m_data->calib_curve;
  
  return true;
}

bool MatthewAnalysis::RetrieveCalibrationCurve(){
  // Constructs calibration curve from coefficients in the config file, then stores in DataModel
  
  const std::string calib_prefix = "CalibCoeff";
  const int pol_order = 6;
  double calib_coefficients[pol_order + 1];
  bool success = true;
  
  // retrieve calibration coefficients from config file
  for (auto i = 0; i < pol_order + 1; ++i){
    success *= m_variables.Get(calib_prefix + std::to_string(i), calib_coefficients[i]);
    std::cout << calib_coefficients[i] << std::endl;
  }
  if(!success){
    Log("MatthewAnalysis::RetrieveCalibrationCurve did not find "+std::to_string(pol_order)
        +" calibration constants on config file!",0,0);
    return false;
  }
  
  // construct 6th order polynomial to convert intensity into concentration
  TF1* calib_curve = new TF1("calib", "pol 6", 0, 0.4);
  calib_curve->SetParameters(calib_coefficients);
  m_data->calib_curve = calib_curve;
  
  return success;
}

bool MatthewAnalysis::RetrieveDarkSubPure(){
  // Retrieves pure trace from .root file then stores it in the DataModel
  
  // pull the filename of the pure water reference trace from the config file
  std::string pure_filename;
  m_variables.Get("pure_filename", pure_filename);
  
  // open the reference file
  TFile pure_file(pure_filename.c_str(), "READ");
  if (! pure_file.IsOpen()){
    Log("MatthewAnalysis::RetrieveDarkSubPure failed to open pure file '"+pure_filename+"'",0,0);
    return false;
  }
  
  //  retrieve the pure water trace (LED spectrum) as a TGraph
  TGraphErrors* dark_subtracted_pure = (TGraphErrors*) pure_file.Get("Graph");
  if(dark_subtracted_pure==nullptr){
    Log("MatthewAnalysis::RetrieveDarkSubPure did not find 'Graph' in pure file '"
        +pure_filename+"'",0,0);
    return false;
  }
  m_data->dark_sub_pure = dark_subtracted_pure;
  
  // construct functional fit. We'll scale the reference pure water, dark subtracted intensity
  // and fit it to the measured data in the LED peak lobes.
  // This allows measurement of the absorption line depth in a way that is less sensitive
  // to variations in absolute scale of measured intensity (e.g. from LED fluctuations)
  const int wave_min = 260, wave_max = 300, numb_of_fitting_parameters = 3;
  TF1* pure_fct = new TF1("pure_fct",
          // TF1 wraps a c++ lambda function that evaluates the reference TGraph at a given wavelength
          [dark_subtracted_pure](double* x, double* par){
          return (par[0] * dark_subtracted_pure->Eval(x[0])) + (par[1] * x[0]) + par[2];},
       wave_min, wave_max, numb_of_fitting_parameters);
  m_data->pure_fct = pure_fct;
  
  return true;
}

bool MatthewAnalysis::ReadyToAnalyse(){
  // Checks if analyse flag has been set by scheduler.
  bool ready = false;
  std::string analyse;
  bool success = m_data->CStore.Get("Analyse", analyse);
  if (success && analyse == "Analyse"){
    m_data->CStore.Remove("Analyse");
    analyse.clear();
    ready = true;
  }
  return ready;
}

std::pair<TTree*, TTree*> MatthewAnalysis::FindDarkAndLEDTrees(){
  // Finds the trees for both dark and led traces; then sorts them into a (dark,led)-ordered pair.
  
  std::pair<TTree*, TTree*> result{nullptr,nullptr};
  
  // this tool only extracts concentration from one trace (one LED) at a time;
  // get the name of the LED whose trace we're analysing
  std::string led_name;
  bool get_ok = m_data->CStore.Get("ledToAnalyse",led_name);
  if(not get_ok){
    Log("MatthewAnalysis::FindDarkAndLEDTrees - Analyse flag found in CStore but no ledToAnalyse",0,0);
    return result;
  }
  
  // loop over TTrees (traces) in DataModel
  for (const auto [name, tree] : m_data->m_trees){
    if (name == led_name){result.second = tree;}                  // signal trace
    else if (boost::iequals(name, "dark")){result.first = tree;}  // dark trace
  }
  
  return result;
}

TGraphErrors MatthewAnalysis::PerformDarkSubtraction(TTree* ledTree, TTree* darkTree){
  // Takes the dark and led trees and returns a TGraphErrors of the dark subtracted trace
  
  // retrieve data from TTrees
  std::vector<double> *led_values= nullptr, *led_wavelengths = nullptr, *led_errors = nullptr;
  std::vector<double>* dark_values = nullptr, *dark_wavelengths = nullptr, *dark_errors = nullptr; 
   
  ledTree->SetBranchAddress("value", &led_values);
  darkTree->SetBranchAddress("value", &dark_values);
  ledTree->SetBranchAddress("wavelength", &led_wavelengths);
  darkTree->SetBranchAddress("wavelength", &dark_wavelengths);
  ledTree->SetBranchAddress("error", &led_errors);
  darkTree->SetBranchAddress("error", &dark_errors);
   
  ledTree->GetEntry(0);
  darkTree->GetEntry(0);
  
  // check we got the data we expected
  const int number_of_points = 2068;    // expected trace size
  if( led_values==nullptr       || led_values->size()       != number_of_points ||
      led_wavelengths==nullptr  || led_wavelengths->size()  != number_of_points ||
      led_errors==nullptr       || led_errors->size()       != number_of_points ||
      dark_values==nullptr      || dark_values->size()      != number_of_points ||
      dark_wavelengths==nullptr || dark_wavelengths->size() != number_of_points ||
      dark_errors==nullptr      || dark_errors->size()      != number_of_points ){
      Log("MatthewAnalysis::PerformDarkSubtraction failed to get data from TTrees!",0,0);
      return TGraphErrors{};
  }
  
  // construct a TGraphErrors of dark-subtracted data
  TGraphErrors result(number_of_points);
  for (auto i = 0; i < number_of_points; ++i){
    result.SetPoint(i, led_wavelengths->at(i), led_values->at(i) - dark_values->at(i));
    result.SetPointError(i, 0, sqrt(pow(led_errors->at(i), 2) + pow(dark_errors->at(i),2)));
  }
  return result;
}

TGraphErrors MatthewAnalysis::RemoveRegion(TGraphErrors& trace){
  // Returns a copy of the input trace with datapoints in the absorption region removed.
  
  const auto lower_bound = 270, upper_bound = 280;
  double temp_x, temp_y, temp_err_x, temp_err_y; 
  
  TGraphErrors result(trace.GetN());
  for (auto i = 0; i < result.GetN(); ++i){
    trace.GetPoint(i, temp_x, temp_y);
    temp_err_x = trace.GetErrorX(i);
    temp_err_y = trace.GetErrorY(i);
    
    if (temp_x < lower_bound || temp_x > upper_bound){
      result.SetPoint(i, temp_x, temp_y);
      result.SetPointError(i, temp_err_x, temp_err_y);
    }
    else {result.RemovePoint(i);}
  }
  return result;
}

double* MatthewAnalysis::FunctionalFit(TGraphErrors& trace){
  // Fits the trace (with absorption region removed) to the pure water trace, and returns the fit parameters
  
  // get the pure-water reference function
  TF1* pure_fct = m_data->pure_fct;
  if(pure_fct==nullptr){
    Log("MatthewAnalysis::FunctionalFit obtained nullptr from DataModel for functional fit TF1",0,0);
    return nullptr;
  }
  
  // do the fit
  TFitResultPtr res = trace.Fit("pure_fct", "RS");
  if(res->IsEmpty() || !res->IsValid() || int(res)!=0){
    Log("MatthewAnalysis::FunctionalFit fit status indicates fit failed!",0,0);
    // see https://root.cern.ch/doc/master/classTH1.html#a7e7d34c91d5ebab4fc9bba3ca47dabdd
    // status section for an interpretation of int(res) values
    return nullptr;
  }
  
  // get fit parameters
  double* fitting_parameters = pure_fct->GetParameters();
  return fitting_parameters;
}
 
TGraphErrors MatthewAnalysis::Scale(TGraphErrors& pure_spectrum, const double* par, const double* par_errors){
  // Creates a TGraphErrors of the pure trace that has been scaled by parameters from a functional fit.
  
  TGraphErrors result(pure_spectrum.GetN());
  
  double x_temp, y_temp;
  
  // scale pure spectrum by fit function parameters
  for (auto i = 0; i < pure_spectrum.GetN(); ++i){
    pure_spectrum.GetPoint(i, x_temp, y_temp);
    result.SetPoint(i, x_temp, par[0] * y_temp + par[1] * x_temp + par[2]);
    result.SetPointError(i, pure_spectrum.GetErrorX(i), sqrt(pow(par[0] * pure_spectrum.GetErrorY(i), 2) + pow(par[1] * pure_spectrum.GetErrorX(i),2)));
  }
  
  return result;
}
 
TGraphErrors MatthewAnalysis::CalculateAbsorbance(TGraphErrors& transmitted, TGraphErrors& received){
  // Creates absorbance spectrum; ie A = log10(received_light / transmitted_light)
  
  TGraphErrors result(transmitted.GetN());
  double trans_x, trans_y, rec_x, rec_y;

  const float LOG10_OF_E = log10(exp(1));
  
  for (auto i = 0; i < transmitted.GetN(); ++i){
    transmitted.GetPoint(i, trans_x, trans_y);
    received.GetPoint(i, rec_x, rec_y);
    if (trans_x > 260 && trans_x < 300){
      result.SetPoint(i, trans_x, log10(trans_y / rec_y ));
      result.SetPointError(i, transmitted.GetErrorX(i), LOG10_OF_E * sqrt(pow((transmitted.GetErrorY(i) / trans_y), 2) + pow(received.GetErrorY(i) / rec_y, 2)));
    }
    else {
      result.SetPoint(i, trans_x, 0);
    }
  }
  return result;
}

std::vector<double> MatthewAnalysis::FindPeakDifference(TGraphErrors& absorbance){
  // Fit Gaussians to the two largest peaks in absorbance and calculate the difference in their heights
  // Since the height of both peaks are linearly related, taking their difference shares the same
  // relation to concentration as peak height, while helping to remove any remaining baseline offset
  
  // fit twin peaks  TODO make fit ranges / functions configurable
  TF1 left_peak = TF1("leftPeak", "gaus", 272, 274), right_peak = TF1("rightPeak", "gaus", 275, 277);
  TFitResultPtr lpf = absorbance.Fit("leftPeak", "0RS");
  TFitResultPtr rpf = absorbance.Fit("rightPeak", "0RS");
  if(lpf->IsEmpty() || !lpf->IsValid() || int(lpf)!=0 ||
     rpf->IsEmpty() || !rpf->IsValid() || int(rpf)!=0){
    Log("MatthewAnalysis::FindPeakDifference fit to absorption peaks failed!",0,0);
    return std::vector<double>{0,0};
  }
  
  // calculate difference and error
  std::vector<double> result;
  result.push_back(left_peak.GetParameter(0) - right_peak.GetParameter(0));
  result.push_back(sqrt(pow(left_peak.GetParError(0),2) + pow(right_peak.GetParError(0),2)));
  
  return result;
}

std::vector<double> MatthewAnalysis::CalculateConcentration(const std::vector<double> peaks_info){
  // Uses the calibration curve to calculate the concentration corresponding to the measured peak difference
  
  // get calibration curve
  TF1* calib_curve = m_data->calib_curve;
  
  std::vector<double> result;
  
  // solve for concentration (x) from absorbance (y), with 0.01 < x < 0.21
  result.push_back(calib_curve->GetX(peaks_info.at(0), 0.01, 0.21));
  
  // estimate the error on measured concentration as the difference in concentrations
  // corresponding to measured absorbance +- the error on absorbance.
  result.push_back(calib_curve->GetX(peaks_info.at(0) + peaks_info.at(1), 0.01, 0.21) -
                   calib_curve->GetX(peaks_info.at(0) - peaks_info.at(1), 0.01, 0.21));
  
  return result;
}
