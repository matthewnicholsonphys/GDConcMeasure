#include "LivePlotter.h"
#include "TGraph.h"
#include "TCanvas.h"

LivePlotter::LivePlotter():Tool(){}


bool LivePlotter::Initialise(std::string configfile, DataModel &data){
  
  if(configfile!="")  m_variables.Initialise(configfile);
  //m_variables.Print();
  m_variables.Get("verbosity", verbosity);
  
  m_data= &data;
  
  return true;
}


bool LivePlotter::Execute(){
  
  // Check if NewMeasurement flag has been set in the CStore by the MatthewAnalysis Tool.
  bool new_measurement;
  bool get_ok = m_data->CStore.Get("NewMeasurement",new_measurement);
  
  // if so, make a TGraph of measurements so far to publish on the website
  if(get_ok && new_measurement){
    
    // can only make a meaningful graph when we have more than one datapoint, really
    int num_of_meas = m_data->concs_and_peakdiffs.size();
    if (num_of_meas > 1){
      
      // concentration vs difference in amplitudes of main absorbance peaks
      // the points on this graph show the sampled range of our calibration plot
      TGraph live_plot(num_of_meas);
      live_plot.SetTitle("Concentration vs Peak Diff;Gd Concentration [%];"
                         "(Absorbance at 273nm) - (Absorbance at 276nm) [no units]");
      
      // fill data
      for (int i = 0; i < num_of_meas; ++i){
        live_plot.SetPoint(i, m_data->concs_and_peakdiffs.at(i).first, m_data->concs_and_peakdiffs.at(i).second);
        // TODO errors are not currently exported by MatthewAnalysis to the datamodel...
        // std::cout << "points: (" << m_data->concs_and_peakdiffs.at(i).first 
        //           << ", " << m_data->concs_and_peakdiffs.at(i).second << ") \n";
      }
      
      // draw and save
      TCanvas c1("c1", "c1", 1000, 1000);
      live_plot.Draw();
      c1.SaveAs("/var/www/html/images/live_conc.png");
      
      // TODO add other live plots, e.g. concentration vs time, or vs measurement.
      // or peak height 1&2 vs measurement etc
      
    }
  }
  
  return true;
}


bool LivePlotter::Finalise(){
  
  return true;
}
