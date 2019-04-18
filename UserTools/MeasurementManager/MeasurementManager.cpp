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

	m_variables.Get("verbose", verbose);

	m_variables.Get("measurement",	measureFile);	//file with list of calibration/measurement
	m_variables.Get("output",	outputFile);	//file in which calibration is saved
	m_variables.Get("base_name",	base_name);	//base_name for calibation
	//m_variables.Get("tree_name", treeName);		//name of output tree
	return true;
}


bool MeasurementManager::Execute()
{
	switch (m_data->mode)
	{
		case state::init:	//about to make measurement, check LED mapping
			Initialise(m_configfile, *m_data);
			measureList = LoadMeasurement();
			im = measureList.begin();	//global iterator
			//std::cout << "Set up for " << measureList.size() << " measurements.\n";
			break;
		case state::measurement:		//turn on led set
			if (im != measureList.end())
			{
				Measure();
				++im;
			}
			else
			{
				m_data->measurementDone = true;
				std::cout << "Measurements completed\n";
			}
			break;
		case state::measurement_done:
			break;
		case state::finalise:
			Finalise();
			break;
	}
	return true;
}


bool MeasurementManager::Finalise()
{
	if (m_data->measurementFile && m_data->measurementFile->IsOpen())
	{
		m_data->measurementFile->Close();
		m_data->measurementFile = NULL;
	}

	for (im = measureList.begin(); im != measureList.end(); ++im)
		m_data->DeleteGdTree(*im);

	measureList.clear();

	return true;
}

std::vector<std::string> MeasurementManager::MeasurementList()
{
	std::vector<std::string> vList;

	std::ifstream cf(measureFile.c_str());
	std::string line;
	while (std::getline(cf, line))
	{
		if (line.find_first_of('#') != std::string::npos)
			line.erase(line.find_first_of('#'));
		
		std::stringstream ssl(line);
		std::string name, word;
		while (ssl)
		{
			if (!std::isalnum(ssl.peek()) && ssl.peek() != '_')
				ssl.ignore();
			else if (ssl >> word)
			{
				if (word.find("time") == std::string::npos)	//no time
					name += word + "_";
			}
		}

		name.erase(name.size()-1);
		vList.push_back(name);
	}

	return vList;
}

std::vector<std::string> MeasurementManager::LoadMeasurement()
{
	//get list of calibrations
	std::vector<std::string> mList = MeasurementList();

	//open read only calibration file
	m_data->measurementFile = new TFile(outputFile.c_str(), "UPDATE");
	if (m_data->measurementFile->IsZombie())
		std::cerr << "Measurement file does not exist!\n";

	//find the measurement tree
	//
	for (im = mList.begin(); im != mList.end(); ++im)
	{
		TTree *gdt = 0;

		if (m_data->calibrationFile->Get(im->c_str()))	//if exists, get it
			gdt = static_cast<TTree*>(m_data->calibrationFile->Get(im->c_str())->Clone());
		//else create new
		
		GdTree *measure = new GdTree(gdt);
		std::string name = base_name + "_" + *im;
		m_data->AddGdTree(name.c_str(), measure);
	}

	return mList;
}

bool MeasurementManager::Measure()
{
	m_data->turnOnLED = *im;
	m_data->measurementName = base_name + "_" + *im;
}
