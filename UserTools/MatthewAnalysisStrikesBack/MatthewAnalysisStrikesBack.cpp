#include "MatthewAnalysisStrikesBack.h"

#include "gad_utils.h"

MatthewAnalysisStrikesBack::MatthewAnalysisStrikesBack():Tool(){}


bool MatthewAnalysisStrikesBack::Initialise(std::string configfile, DataModel &data){

  if(configfile!="")  m_variables.Initialise(configfile);
  //m_variables.Print();

  m_data= &data;

  /*
    
    DO THIS FOR BOTH 275 LEDs

    1. Get pure and high conc (dark subtracted) (from file or database)
    2. Create simple FunctionalFit
    3. Use simple fit on pure high conc
    4. Calculate GDAbsTerm using result from simplefit
    5. Create Abs Fit using ratio of simple fit result and high conc
    6. Create CombinedFit using GDAbs and pure
   */

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
  
  const TGraph test = GetDarkSubtractFromFile("test", "name", 0);
  
  return true;
}


bool MatthewAnalysisStrikesBack::Finalise(){

  return true;
}
