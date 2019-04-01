#include "MeasurementManager.h"

MeasurementManager::MeasurementManager() : Tool()
{
}

bool MeasurementManager::Initialise(std::string configfile, DataModel &data)
{
	if (configfile != "")
	{
		m_configfile = configfile;
		m_variables.Initialise(configfile);
	}
	//m_variables.Print();

	m_data= &data;

	Configure();
	return true;
}


bool MeasurementManager::Execute()
{
	switch (m_data->mode)
	{
		case state::init:	//about to make measurement, check LED mapping
			Initialise(m_configfile, *data);
			measureList = LoadMeasurement();
			measureCount = 0;
			std::cout << "Set up for " << measureList.size() << " measurements.\n";
			break;
		case state::measurement:		//turn on led set
			if (Measure())			//cheak if there are measurements to take
				m_data->mode = state::measurement;
			else
				m_data->mode = state::measurement_done;
			break;
		case state::measurement_done:
			m_data->outputFile = outFile;
			break;
		case state::finalise:
			Finalise();
			break;
		default:		//turn off in any other state
			TurnOff();
			break;
	}
	return true;
}


bool MeasurementManager::Finalise()
{
	measureList.clear();
	return true;
}

void MeasurementManager::Configure()
{
	m_variables.Get("verbose", verbose);

	m_variables.Get("measurement", measureFile);	//file with list of measurements to take
	m_variables.Get("output", outFile);	//file with list of measurements to take
	//m_variables.Get("tree_name", treeName);		//name of output tree

	Log("LEDManager: LED measurement will be read from " + measureFile, 1, verbose);
}

std::vector<std::string> MeasurementManager::LoadMeasurement()
{
	std::vector<std::string> vList;

	std::ifstream (measureFile.c_str());
	std::string line;
	while (std::getline(measureFile, line))	//skip to line measureCount+1
	{
		if (line.find_first_of('#') != std::string::npos)
			line.erase(line.find_first_of('#'));
		vList.push_back();
	}

	return vList;
}

bool MeasurementManager::Measure()
{
	if (m_data->LED_name.size() && measureCount < measureList.size())	//led mapping has been loaded
	{									//and there are still measruements to take
		std::string led;
		std::string out;
		std::stringstream ssl(measureList.at(measureCount));		//setup binary value in data model
		while(ssl >> led)
		{
			out = led + "_";
			m_data->ledON = m_data->ledON | m_data->LED_name[led];
		}

		if (!out.empty())
			out.erase(erase.size()-1);

		if (!m_data->GetGdTree(out))					//create a tree for measurement if it doesn't exist
		{
			GdTree *measure = new GdTree(out, outFile);	//get tree of measurement
			m_data->AddGdTree(out, measure);
		}

		++measureCount;
	}
	else
		return false;
}
