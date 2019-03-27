#include "Power.h"

Power::Power() : Tool()
{
}

bool Power::Initialise(std::string configfile, DataModel &data)
{
	if(configfile!="")  m_variables.Initialise(configfile);
	//m_variables.Print();

	m_data= &data;

	return true;
}

bool Power::Execute()
{
	switch (m_data->mode)
	{
		case state::power_off:
			TurnOff();
			break;
		case state::power_up:
			TurnOn();
			break;
	}
	return true;
}


bool Power::Finalise()
{
	return true;
}
