#include "Pump.h"

Pump::Pump() : Tool()
{
}

bool Pump::Initialise(std::string configfile, DataModel &data)
{
	if(configfile!="")  m_variables.Initialise(configfile);
	//m_variables.Print();

	m_data= &data;

	return true;
}

bool Pump::Execute()
{
	switch (m_data->mode)
	{
		case state::change_water:
			TurnOff();
			break;
		case state::power_up:
			TurnOn();
			break;
	}
	return true;
}


bool Pump::Finalise()
{
	return true;
}
