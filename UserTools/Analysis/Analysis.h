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

	private:

		std::vector<double> darkTrace, pureTrace;

};


#endif
