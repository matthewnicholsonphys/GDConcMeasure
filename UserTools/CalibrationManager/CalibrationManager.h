#ifndef CalibrationManager_H
#define CalibrationManager_H

#include <string>
#include <iostream>

#include "Tool.h"

class CalibrationManager: public Tool
{
	public:

		CalibrationManager();
		bool Initialise(std::string configfile,DataModel &data);
		bool Execute();
		bool Finalise();

		void Configure();
		void LoadCalibration();
		void NewCalibration();
		bool IsCalibrated();

	private:

		std::string m_configfile;
		int verbose;

		std::string calibFile;		//file in which calibration list is
		std::string outputFile;		//root file in which calibration will saved
		std::string base_name;		//base name for calibration
		std::string concFuncName;	//name of concentration function
		std::string err_FuncName;	//name of uncertainity function

		std::vector<std::string> calibList, updateList;
		std::vector<std::string>::iterator im;

		std::map<std::string, int> timeUpdate;


};


#endif
