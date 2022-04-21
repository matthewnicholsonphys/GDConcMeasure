#ifndef SaveToDB_H
#define SaveToDB_H

#include <string>
#include <iostream>

#include "Tool.h"

class SaveToDB: public Tool {
	
	public:
	
	SaveToDB();
	bool Initialise(std::string configfile,DataModel &data);
	bool Execute();
	bool Finalise();
	
	private:
	int max_webpage_records=5;
	
	// one method per Tool to put that Tool's results into the DB
	bool MatthewAnalysis();
	bool BenPower();
	bool Valve();
	bool BenLED();
	bool BenSpectrometer();
	bool TraceAverage();
	bool SaveTraces();
	bool MarcusScheduler();
	bool RoutineCalibration();  // placeholder, Tool TODO
	
	// helper functions for converting data into json arrays / objects to store in DB
	std::string BuildJson(TGraphErrors* gr, std::pair<double, double> range, bool inside_data);
	std::string BuildJson(TGraphErrors* gr);
	std::string BuildJson(TGraph* gr);
	std::string BuildJson(double* arr, double* err, int n);
	std::string BuildJson(double* arr, int n);
	std::string BuildJson(int* arr, int n);
	std::string BuildJson(std::vector<std::string> arr);
	std::string BuildJson(TFitResultPtr& fitresptr);
	std::string CastJsonb(std::string& in);
	std::string CastJsonb(int in);
	
	std::vector<std::string> field_names;
	std::vector<std::string> criterions;
	std::vector<char> comparators;
	std::string error_ret;
	std::string query_string;
	
	int verbosity=1;
	int v_error=0;
	int v_warning=1;
	int v_message=2;
	int v_debug=3;
};


#endif
