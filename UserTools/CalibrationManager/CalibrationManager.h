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

		int updateTime;		//number of seconds every time calibration should happen
		std::string calibFile;	//file in which calibration is saved
		std::string treeName;	//name of calibration tree
		std::string concFunc;	//name of concentration function
		std::string err_Func;	//name of uncertainity function
		std::string fileList;	//file in which calibration is saved
		std::string calibLED;	//name of LED used to do calibration






};


#endif
