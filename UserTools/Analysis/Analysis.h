#ifndef Analysis_H
#define Analysis_H

#include <string>
#include <iostream>

#include "Tool.h"

class Analysis: public Tool
{
	public:

		Analysis();
		bool Initialise(std::string configfile,DataModel &data);
		bool Execute();
		bool Finalise();

		std::vector<double> AverageTrace(bool darkRemove);
		std::vector<double> AbsorbTrace(const std::vector<double> &avgTrace);

		bool Absorbance(std::vector<double> &avgTrace, std::vector<double> &absTrace, int &i1, int &i2);
		void CalibrationTrace(double gdconc, double gd_err);
		void MeasurementTrace();

		void LinearFit();
		void FindPeakDeep(const std::vector<double> &vTrace, std::vector<unsigned int> &iPeak, std::vector<unsigned int> &iDeep);

	private:

		std::vector<double> darkTrace, pureTrace;

};


#endif
