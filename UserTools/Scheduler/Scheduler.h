#ifndef Scheduler_H
#define Scheduler_H

#include <string>
#include <iostream>

#include "Tool.h"
#include "boost/date_time/gregorian/gregorian.hpp"
#include "boost/date_time/posix_time/posix_time.hpp"
#include "boost/chrono.hpp"

class Scheduler: public Tool
{
	public:
		Scheduler();

		bool Initialise(std::string configfile,DataModel &data);
		bool Execute();
		bool Finalise();

		void Configure();
		bool IsCalibrated();
		bool IsCalibrationDone();
		bool IsMeasurementDone();

		boost::posix_time::second_clock Wait(double t = 0.0);

	private:
		boost::posix_time::ptime last;


};


#endif
