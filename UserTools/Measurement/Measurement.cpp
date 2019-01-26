#include "Measurement.h"

Measurement::Measurement() : Tool()
{
}

bool Measurement::Initialise(std::string configfile, DataModel &data)
{
	if(configfile!="")  m_variables.Initialise(configfile);
	//m_variables.Print();

	m_data= &data;

	return true;
}


bool Measurement::Execute()
{
	switch (m_data->mode)
	{
		case state::init:	//about to make measurement, check LED mapping
			Configure();
			break;
		case state::measure:		//turn on led set
			if (Measure())		//if TurnOn is false, measurement is finished or there is a uncaught problem
				m_data->mode = state::measure_start;
			else
				m_data->mode = state::measure_stop;
			break;
		case state::analyse:	//wait for measurement
			break;
		default:		//turn off in any other state
			TurnOff();
			break;
	}
	return true;
}


bool Measurement::Finalise()
{
	return true;
}

void LEDmanager::Configure()
{
	m_variables.Get("verbose", verbose);

	m_variables.Get("measurement", measureList);

	Log("LEDManager: LED measurement will be read from " + measureList, 1, verbose);
}

bool Calibration::Measure()
{
	if (m_data->LED_name.size())	//led mapping has been loaded
	{
		std::ifstream (measureList.c_str());
		std::string line;
		int i = -1;
		while (i < measureCount && std::getline(measureList, line))	//skip to line measureCount+1
			if (line[0] != '#')
				++i;

		unsigned int ledON = 0;
		if (i == measureCount)
		{
			if (line.find_first_of('#') != std::string::npos)
				line.erase(line.find_first_of('#'));
			std::stringstream ssl(line);		//setup binary value
			while(ssl >> line)
				m_data->ledON = m_data->ledON | m_data->LED_name[line];

			++measureCount;
		}
		else
			return false;

		if (ledON != ledONstatus)
		{
			ledONstatus = ledON;
			return TurnLEDArray(ledONstatus);	//if everything is ok, this returns true
		}
		else
			return true;		//continue measuring
	}
	else
		std::cout << "LED mapping not done\n";
}
