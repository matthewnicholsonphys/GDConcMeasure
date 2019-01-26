#include "Writer.h"

Writer::Writer() : Tool()
{
}

bool Writer::Initialise(std::string configfile, DataModel &data)
{
	if(configfile!="")  m_variables.Initialise(configfile);
	//m_variables.Print();

	m_data= &data;

	return true;
}

bool Writer::Execute()
{
	switch (m_data->mode)
	{
		case state::init:
			darkTrace.clear();
			pureTrace.clear();
			vTraceCollect.clear();
			break;
		case state::write_measure:
			break;
		case state::write_calibrate:
			break;
		default:
			break;
	}

	return true;
}

bool Writer::Finalise()
{
	return true;
}
