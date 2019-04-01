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
		bool Measure();
		std::vector<std::string> LoadMeasurement();

	private:

		std::vector<std::string> measureList;

		std::string m_configfile;
		int verbose;

		std::string measureFile;	//file with list of measurements to take
		std::string outFile;		//file with list of measurements to take
		std::string treeName;		//name of output tree
};

#endif
