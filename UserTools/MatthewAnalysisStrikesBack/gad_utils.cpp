#include "gad_utils.h"

SimplePureFunc::SimplePureFunc(const TGraph& pds)
  : pure_dark_subtracted{pds} {
  fit_min = wave_min;
  fit_max = wave_max;
}

double SimplePureFunc::Evaluate(double* x, double* p){
  return (p[PURE_SCALING] + p[PURE_FIRST_ORDER_CORRECTION] * x[0] ) * (pure_dark_subtracted.Eval(x[0] - p[PURE_TRANSLATION])) +
	  p[LINEAR_BACKGROUND_GRADIENT] * x[0]
	  + p[LINEAR_BACKGROUND_OFFSET];
}


CombinedGdPureFunc::CombinedGdPureFunc(const TGraph& pds, const TGraph& a)
  : pure_dark_subtracted{pds}, spec_abs{a} {
  fit_min = wave_min;
  fit_max = wave_max;
}

double CombinedGdPureFunc::Evaluate(double* x, double* p){
  return (p[PURE_SCALING] + p[PURE_FIRST_ORDER_CORRECTION] * x[0]) * pure_dark_subtracted.Eval(x[0] - p[PURE_TRANSLATION]) +
    p[ABS_SCALING] * spec_abs.Eval(x[0] - p[ABS_TRANSLATION]) +
    p[FIRST_ORDER_BACKGROUND] * x[0] +
    p[ZEROTH_ORDER_BACKGROUND];
}

CombinedGdPureFunc_DATA::CombinedGdPureFunc_DATA(const TGraph& pds, const TGraph& a)
  : pure_dark_subtracted{pds}, rat_abs{a} {
  fit_min = wave_min;
  fit_max = wave_max;
}
 
double CombinedGdPureFunc_DATA::Evaluate(double* x, double* p){
  return (p[PURE_SCALING])  // p[PURE_FIRST_ORDER_CORRECTION] * x[0]
    * pure_dark_subtracted.Eval(p[PURE_STRETCH] * (x[0] - 276) + 276 - p[PURE_TRANSLATION]) * (std::max(0.0, (1-p[ABS_SCALING]*rat_abs.Eval(x[0]))) +
											       p[SECOND_ORDER_BACKGROUND] *  (x[0]-276) * (x[0]-276) +
											       p[FIRST_ORDER_BACKGROUND] * (x[0]-276) +
											       p[ZEROTH_ORDER_BACKGROUND]);
}
  
// pure + abs that spec sees (0 outside of abs region) + pol1 - wiggled (non-trivial / impossible)

// pure * ratio absorbance (physics) + pol1 (wiggling all of this)
// how do we transform the pure: scale, shift, non-linear transform
// r = pure / gd => gd = pure / r

AbsFunc::AbsFunc(const TGraph& a)
  : abs_ds{a} {
  fit_min = abs_region_low;
  fit_max = abs_region_high;
}

double AbsFunc::Evaluate(double* x, double* p){
  return p[ABS_SCALING] * abs_ds.Eval(x[0] - p[ABS_TRANSLATION]) +
    p[THIRD_BACKGROUND] * x[0] * x[0] * x[0] + 
    p[SECOND_BACKGROUND] * x[0] * x[0] + 
    p[FIRST_BACKGROUND] * x[0] +
    p[ZEROTH_BACKGROUND];
}

FunctionalFit::FunctionalFit(Func* func_class_ptr, const std::string& fcn) : fit_name{fcn} {
  if (func_class_ptr->fit_min == 0 ||
      func_class_ptr->fit_max == 0 ||
      func_class_ptr->n_fit == 0){
    throw std::invalid_argument("FunctionalFit::FromFunctionClass: FIT REGION OR FIT DIMENSION NOT VALID!!!");
  }
  
  fit_funct = TF1(fit_name.c_str(),
		  func_class_ptr,
		  &Func::Evaluate,
		  func_class_ptr->fit_min, func_class_ptr->fit_max, func_class_ptr->n_fit,
		  fit_name.c_str(),"Evaluate");

  if (!fit_funct.IsValid()){
    throw std::runtime_error("FunctionalFit::FunctionalFit FIT FUNCTION IS NOT VALID!!!\n");
  }
}

void FunctionalFit::SetFitParameters(const std::vector<double>& pv){
  if (static_cast<int>(pv.size()) != fit_funct.GetNpar()){
    throw std::invalid_argument("FunctionalFit::SetFitParameters: WRONG NUMBER OF INITIAL FIT PARAMETER VALUES!!!\n"); 
  }
  fit_funct.SetParameters(pv.data());
  return;
}

void FunctionalFit::SetFitParameterRanges(const std::vector<std::pair<double, double>>& fpr){
  if (static_cast<int>(fpr.size()) != fit_funct.GetNpar()){
    throw std::invalid_argument("FunctionalFit::SetFitParameterRanges: WRONG NUMBER OF INITIAL FIT PARAMETER RANGES!!!\n"); 
  }
  for (int i = 0; i < fit_funct.GetNpar(); ++i){
    fit_funct.SetParLimits(i, fpr.at(i).first, fpr.at(i).second);
  }
}


void FunctionalFit::PerformFitOnData(TGraph data, bool interactive){
  fit_funct.SetNpx(10000);
  if (interactive){
    TApplication app = TApplication("app", 0, 0);
    TCanvas c1 = TCanvas("fiddle_canv", "fiddle", 1280, 1024);
    data.Draw("A*L");
    TFitResultPtr res = data.Fit(&fit_funct, "RSQ"); // keep this one
    while(gROOT->FindObject("fiddle_canv") != 0){
      c1.Modified();
      c1.Update();
      gSystem->ProcessEvents();
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
  }
  else {
    TFitResultPtr res = data.Fit(&fit_funct, "NRS"); // keep this one
  }  
  // if (res->IsEmpty() || !res->IsValid() || res->Status() != 0){
  //   throw std::runtime_error("FunctionalFit::PerformFitOnData: FIT NOT SUCCESSFUL!!!\n");
  //   }
  fitted = true;
}

void FunctionalFit::SetExampleGraph(const TGraph& e){
  if (e.GetN() != number_of_points){
    throw std::invalid_argument("FunctionalFit::SetExampleGraph: EXAMPLE HAS WRONG NUMBER OF POINTS!!!\n");
  }
  example = e;
}

double FunctionalFit::GetParameterValue(const int& p) const {
  if (!fitted){
    throw std::runtime_error("FunctionalFit::GetGraph - NOT YET FITTED!!!\n");
  }
  return fit_funct.GetParameter(p);
}

double FunctionalFit::GetChiSquared() const {
  if (!fitted){
    throw std::runtime_error("FunctionalFit::GetGraph - NOT YET FITTED!!!\n");
  }
  TF1 local_ff = fit_funct;
  return local_ff.GetChisquare();
}

TGraph FunctionalFit::GetGraph() const {
  if (!fitted){
    throw std::runtime_error("FunctionalFit::GetGraph - NOT YET FITTED!!!\n");
  }
  if (example.GetN() != number_of_points){
    throw std::runtime_error("FunctionalFit::GetGraph - NO EXAMPLE FOR POPULATING SET!!!\n");
  }
  TF1 local_ff = fit_funct;
  TGraph result(number_of_points);
  for (int i = 0; i < example.GetN(); ++i){
    result.SetPoint(i, example.GetPointX(i), local_ff.Eval(example.GetPointX(i)));
  }
  return result;
}
  
TGraph FunctionalFit::GetGraphOfJust(const std::vector<int>& p) const {
  if (!fitted){
    throw std::runtime_error("FunctionalFit::GetGraphOfJust - NOT YET FITTED!!!\n");
  }
  if (example.GetN() != number_of_points){
    throw std::runtime_error("FunctionalFit::GetGraph - NO EXAMPLE FOR POPULATING SET!!!\n");
  }
  TF1 local_ff = fit_funct;
  for (const auto param : p){
    if (param > local_ff.GetNpar() || param < 0){
      throw std::invalid_argument("FunctionalFit::GetGraphOfJust - INVALID PARAMETER CHOICE!!!\n");
    }
  }
  for (int i = 0; i < local_ff.GetNpar(); ++i){
    if (std::find(p.begin(), p.end(), i) == p.end()){
      local_ff.SetParameter(i, 0);
    }
  }
  TGraph result(number_of_points);
  for (int i = 0; i < example.GetN(); ++i){
    result.SetPoint(i, example.GetPointX(i), local_ff.Eval(example.GetPointX(i)));
  }
  return result;
}

TGraph FunctionalFit::GetGraphExcluding(const std::vector<int>& p) const {
  if (!fitted){
    throw std::runtime_error("FunctionalFit::GetGraphExcluding - NOT YET FITTED!!!\n");
  }
  if (example.GetN() != number_of_points){
    throw std::runtime_error("FunctionalFit::GetGraph - NO EXAMPLE FOR POPULATING SET!!!\n");
  }
  TF1 local_ff = fit_funct;
  for (const auto param : p){
    if (param > local_ff.GetNpar() || param < 0){
      throw std::invalid_argument("FunctionalFit::GetGraphExcluding - INVALID PARAMETER CHOICE!!!\n");
    }
  }
  for (int i = 0; i < local_ff.GetNpar(); ++i){
    if (std::find(p.begin(), p.end(), i) != p.end()){
      local_ff.SetParameter(i, 0);
    }
  }
  TGraph result(number_of_points);
  for (int i = 0; i < example.GetN(); ++i){
    result.SetPoint(i, example.GetPointX(i), local_ff.Eval(example.GetPointX(i)));
  }
  return result;
}

TGraph DarkSubtractFromTreePtrs(TTree* led_ptr, TTree* dark_ptr, const int dark_entry = -1){
  if (led_ptr == nullptr || dark_ptr ==  nullptr){
    throw std::invalid_argument("DarkSubtractFromTreePtrs: one or more input trees is nullptr!!!\n");
  }

  std::vector<double> *led_values = nullptr, *dark_values = nullptr, *wavelengths = nullptr;
  const bool set_branch_success = 
    (led_ptr->SetBranchAddress("value", &led_values) == 0) &&
    (led_ptr->SetBranchAddress("wavelength", &wavelengths) == 0) &&
    (dark_ptr->SetBranchAddress("value", &dark_values) == 0);
  if (!set_branch_success){
    throw std::runtime_error("DarkSubtractFromTreePtrs: error when setting one or more branch addresses!!!\n");
  }
  
  const bool get_entry_success = 
    led_ptr->GetEntry(0) && 
    dark_entry == -1 ? dark_ptr->GetEntry(dark_ptr->GetEntries()-1) : dark_ptr->GetEntry(dark_entry); 
  if (!get_entry_success){
    throw std::runtime_error("DarkSubtractFromTreePtrs: failed to get led or dark entry!!!\n");
  }
  if (static_cast<int>(led_values->size()) != number_of_points ||
      static_cast<int>(dark_values->size()) != number_of_points ||
      static_cast<int>(wavelengths->size()) != number_of_points){
    throw std::runtime_error("DarkSubtractFromTreePtrs: retrieved vectors are incorrect size!!!\n");
  }
  
  TGraph result = TGraph(number_of_points);
  for (int i = 0; i < number_of_points; ++i){
    result.SetPoint(i, wavelengths->at(i), led_values->at(i) - dark_values->at(i));
  }
  return result;
}

TGraph GetDarkSubtractFromFile(const std::string fname, const std::string led_name, const int& dark_offset = -1){
  TFile* file_ptr = TFile::Open(fname.c_str(), "OPEN");
  if (!file_ptr->IsOpen()){
    throw std::invalid_argument("GetDarkSubtractFromFile: FAILED TO OPEN FILE" + fname + "!!!\n");
  }
  TGraph result =  DarkSubtractFromTreePtrs(static_cast<TTree*>(file_ptr->Get(led_name.c_str())),
				  static_cast<TTree*>(file_ptr->Get(dark_name.c_str())),
				  dark_offset);
  file_ptr->Close();
  return result;
}

TGraph RemoveRegion(const TGraph& g, const double& low, const double& high){
  if (g.GetN() != number_of_points){
    throw std::invalid_argument("RemoveRegion: INPUT TGraph HAS INCORRECT NUMBER OF POINTS!!!\n");
  }
  if (g.GetPointX(0) > low || g.GetPointX(g.GetN()-1) < high){
    throw std::invalid_argument("RemoveRegion: INVALID REGION BOUNDS!!!\n");
  }
  TGraph result =  TGraph(number_of_points);
  for (int i = number_of_points-1; i > -1; --i){
    const double x_temp = g.GetPointX(i);
    if (x_temp < low || x_temp > high){
      result.SetPoint(i, x_temp, g.GetPointY(i));
    }
    else {
      result.RemovePoint(i);
    }
  }
  return result;
}

template<class L>
TGraph BinaryOperation(const TGraph& a, const TGraph& b, const std::string& name, const L& op){
  if (a.GetN() != b.GetN()){
    throw std::invalid_argument((name +": INPUT TGraphs HAVE UNEQUAL NUMBER OF POINTS, a has "+a.GetN()+"  b has "+b.GetN()+"!!!\n"));
  }
  const int N = a.GetN();
  TGraph result(N);
  for (int i = 0; i < N; ++i){
    result.SetPoint(i, 0.5 * (a.GetPointX(i) + b.GetPointX(i)), op(a,b,i));
  }
  return result;
}

TGraph PWRatio(const TGraph& n, const TGraph& d){
  return BinaryOperation(n, d, "PWRatio",
			 [](const TGraph& _n, const TGraph& _d, int _i){
			   return _n.GetPointY(_i) / _d.GetPointY(_i);});
}

TGraph PWMultiply(const TGraph& n, const TGraph& d){
  return BinaryOperation(n, d, "PWMultiply",
			 [](const TGraph& _n, const TGraph& _d, int _i){
			   return _n.GetPointY(_i) * _d.GetPointY(_i);});
}


TGraph PWPercentageDiff(const TGraph& n, const TGraph& d){
  return BinaryOperation(n, d, "PWDifference",
			 [](const TGraph& _n, const TGraph& _d, int _i){
			   return _n.GetPointY(_i) / _d.GetPointY(_i) - 1;});  
}

TGraph PWDifference(const TGraph& n, const TGraph& d){
  return BinaryOperation(n, d, "PWDifference",
			 [](const TGraph& _n, const TGraph& _d, int _i){
			   return _n.GetPointY(_i) - _d.GetPointY(_i);});  
}

double SuccessMetric(const TGraph& res){
  TGraph abs_w_cut(res.GetN());
  for (int i = 0; i < res.GetN(); ++i){
      if (res.GetPointX(i) > wave_min && res.GetPointX(i) < wave_max){
        abs_w_cut.SetPoint(i, res.GetPointX(i), abs(res.GetPointY(i)));
      }
    }
    return abs_w_cut.Integral();
}

double GetPeakDiff(const TGraph& abs){
  return abs.Eval(273) - abs.Eval(276);
}

double GetPeakRatio(const TGraph& abs){
  return abs.Eval(273) / abs.Eval(276);
}

TGraph TrimGraph(const TGraph& g, double l, double h){
  TGraph result =  TGraph(number_of_points);
  for (int i = number_of_points-1; i > -1; --i){
    const double x_temp = g.GetPointX(i);
    if (x_temp > h || x_temp < l){
      result.RemovePoint(i);
    }
    else {
      result.SetPoint(i, x_temp, g.GetPointY(i));
    }
  }
  return result;
}

TGraph CalculateGdAbs(const TGraph& a, const TGraph& b){
  // a - pure, b - gd
  std::vector<double> vals;
  TGraph result = TGraph(number_of_points);
  for (int i = 0; i < a.GetN(); ++i){
    if (a.GetPointX(i) > abs_region_high || a.GetPointX(i) < abs_region_low){
      result.SetPoint(i, a.GetPointX(i), 0);
    }
    else {
      vals.push_back(1 - (b.GetPointY(i) / a.GetPointY(i)));
      result.SetPoint(i, a.GetPointX(i), 1 - (b.GetPointY(i) / a.GetPointY(i)));
    }
  }
  std::vector<double>::iterator scale_it = std::max_element(vals.begin(), vals.end());
  for (int i = 0; i < a.GetN(); ++i){
    result.SetPointY(i, result.GetPointY(i) / *scale_it);
  }
  return result;
}

TGraph Normalise(TGraph a){
  if (a.GetN() != number_of_points){
    std::invalid_argument("Normalise: INPUT TGraphs HAS WRONG NUMBER OF POINTS!!!\n");
  }
  double max = 0;
  for (int i = 0; i < number_of_points; ++i){
    if (a.GetPointY(i) > max){max = a.GetPointY(i);}
  }
  a.Scale(1/max);
  return a;
}

TGraph ZeroNegative(TGraph a){
  TGraph result(number_of_points);
  for (int i = 0; i < number_of_points; ++i){
    result.SetPoint(i, a.GetPointX(i), abs(a.GetPointY(i)) + 0.01);
  }
  return result;
}
