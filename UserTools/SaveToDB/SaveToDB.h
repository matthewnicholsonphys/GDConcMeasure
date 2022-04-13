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
	
	
};


#endif
