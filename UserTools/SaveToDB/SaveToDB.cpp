#include "SaveToDB.h"

SaveToDB::SaveToDB():Tool(){}


bool SaveToDB::Initialise(std::string configfile, DataModel &data){
	
	if(configfile!="")  m_variables.Initialise(configfile);
	//m_variables.Print();
	
	m_data= &data;
	
	return true;
}


bool SaveToDB::Execute(){
	
	// see if we have new data to add to DB
	
	bool new_measurement;
	bool get_ok = m_data->CStore.Get("NewMeasurement",new_measurement);
	if(get_ok && new_measurement){
		
		// retrieve data to put into database
		
//		ref260peak	json{amplitude,error}
//		on260peakraw	json{amplitude,error}
//		on260peakdarksub	json{amplitude,error}
//		transm260peak	real
//			
//		functionalfitpars	json {pars}
//		goodness	real
//		concentration	real
//		error	real
//		baseline	{mean,sigma}
//		peak260	{gaussian mean,amp,sigma}
//		peak275	{gaussian mean,amp,sigma}
//		peak385	{gaussian mean,amp,sigma}
//		peakR	{gaussian mean,amp,sigma}
//		peakG	{gaussian mean,amp,sigma}
//		peakB	{gaussian mean,amp,sigma}
//		peakW	???
		
	}
	
	return true;
}


bool SaveToDB::Finalise(){
	
	return true;
}
