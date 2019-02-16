#ifndef CalibrationManager_H
#define CalibrationManager_H

#include <string>
#include <iostream>

#include <vector>
#include <map>

#include "TList.h"
#include "TF1.h"
#include "TF2.h"

#include "Tool.h"

class CalibrationManager: public Tool
{
	public:

		CalibrationManager();
		bool Initialise(std::string configfile,DataModel &data);
		bool Execute();
		bool Finalise();

		std::vector<std::string> CalibrationList();
		std::vector<std::string> LoadCalibration(std::vector<std::string> &uList);
		void Load(std::string name, std::string type);
		bool IsUpdate(std::string name, int timeUpdate);
		void NewCalibration();
		bool Calibrate();

	private:

		std::string m_configfile;
		int verbose;

		std::string calibFile;		//file in which calibration list is
		std::string outputFile;		//root file in which calibration will saved
		std::string base_name;		//base name for calibration
		std::string concFuncName;	//name of concentration function
		std::string err_FuncName;	//name of uncertainity function
		std::string concTreeName;

		std::vector<std::string> calibList, updateList;
		std::vector<std::string>::iterator ic;

		std::map<std::string, int> timeUpdate;
};


#endif
