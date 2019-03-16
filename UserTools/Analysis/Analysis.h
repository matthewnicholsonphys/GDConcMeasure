#ifndef Analysis_H
#define Analysis_H

#include <string>
#include <iostream>

#include <vector>
#include <deque>

#include "TGraphErrors.h"
#include "TFitResult.h"
#include "TFitResultPtr.h"

#include "Tool.h"

//sort the order of peaks and deeps
struct Sort
{
	std::vector<double> vX;
	int peakORdeep;
	bool operator()(int i, int j) const { return peakORdeep*(vX.at(i) - vX.at(j)) > 0; }

	Sort(const std::vector<double> &vv, int PoD = 1) : vX(vv), peakORdeep(PoD) {}
};


class Analysis: public Tool
{
	public:

		Analysis();
		bool Initialise(std::string configfile,DataModel &data);
		bool Execute();
		bool Finalise();

		std::vector<double> PureTrace();
		std::vector<double> AverageTrace(bool darkRemove);
		std::vector<double> AbsorbTrace(const std::vector<double> &avgTrace);
		std::vector<double> Absorbance(const std::vector<double> &avgTrace, int &i1, int &i2);
		void CalibrationTrace(double gdconc, double gd_err);
		void CalibrationComplete();
		void MeasurementTrace();
		void LinearFit();
		void FindPeakDeep(const std::vector<double> &vTrace,
				  std::vector<int> &iPeak,
				  std::vector<int> &iDeep);

		void FillAbsorbance(GdTree *tree, std::vector<double> &abst,
				    int i1, int i2, bool stamp = true);
		ULong64_t TimeStamp(int &Y, int &M, int &D, int &h, int &m, int &s);

	private:

		std::string m_configfile;
		int verbose;

		std::vector<double> darkTrace, pureTrace;

		bool mustComplete;
};


#endif
