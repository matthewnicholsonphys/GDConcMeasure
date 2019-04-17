#ifndef MeasurementManager_H
#define MeasurementManager_H

#include <string>
#include <iostream>

#include "Tool.h"

class MeasurementManager: public Tool
{
	public:

		MeasurementManager();
		bool Initialise(std::string configfile,DataModel &data);
		bool Execute();
		bool Finalise();

		void Configure();
		std::vector<std::string> MeasurementList();
		std::vector<std::string> LoadMeasurement();
		bool Measure();

	private:

		std::string m_configfile;
		int verbose;

		std::string measureFile;	//file with list of measurements to take
		std::string outputFile;		//file with list of measurements to take
		std::string base_name;		//name of output tree

		std::vector<std::string> measureList;
		std::vector<std::string>::iterator im;
};

#endif
