#include "Calibration.h"

Calibration::Calibration():Tool(){}


bool Calibration::Initialise(std::string configfile, DataModel &data)
{

	if(configfile!="")  m_variables.Initialise(configfile);
	//m_variables.Print();

	m_data= &data;

	return true;
}


bool Calibration::Execute()
{
	switch (m_data->mode)
	{
		case state::init:	//about to make measurement, check LED mapping
			LoadCalibration();
			m_data->calibrated = IsCalibrated();
			break;
		case state::calibrate:		//turn on led set
			std::string input;
			std::cout << "Concentration loaded (0 for pure water, any other key to quit): ";
			std::cin >> input;
			if (!std::isdigit(input[0]))
			{
				std::cout << "Calibration complete\n";
				m_data->mode = state::calibrate_fit;
			}
			else
			{
				m_data->ledON = m_data->LED_name["led265"];	//turn on LED UV
				double gd = std::strtod(input, NULL);
				if (gd == 0)
					m_data->mode = state::calibrate_pure;
				else
					m_data->mode = state::calibrate_gd;
			}
			break;
		default:
			break;
	}

	return true;
}

bool Calibration::Finalise()
{
	return true;
}

void Calibration::FitCalibration()
{
}

/*
double Calibration::DefineCalibration()
{
	std::ifstream cal(calibList.c_str());
	std::string line;
	int i = 0;
	while (i < calibCount && std::getline(cal, line))	//skip to line measureCount+1
		if (line[0] != '#' && !std::isspace(line[0]))
			++i;

	double gd;
	if (i == calibCount)
	{
		if (line.find_first_of('#') != std::string::npos)
			line.erase(line.find_first_of('#'));

		++calibCount;
		return strtod(line, NULL);
	}
	else
		return -1.0;
}
*/


bool Calibration::IsCalibrated()
{
	std::string calibFile;
	int calibUpdate;
	m_variables.Get("calibration", calibFile);	//file in which calibration is saved
	m_variables.Get("update",      calibUpdate);	//number of seconds every time calibration should happen
	m_variables.Get("routine", calibList);	//file in which calibration is saved

	std::string cmd = "stat -c %y " + calibFile + " > .tmp_calibration";
	std::system(cmd.c_str());
	std::ifstream inCalibration(".tmp_calibration");
	boost::posix_time::ptime lastUpdate;	//empty?
	if (std::getline(inCalibration, cmd))
		lastUpdate = boost::posix_time::ptime(time_from_string(updateTime));
	//updateTime = std::strtol(cmd.c_str(), NULL, 10);

	if (lastUpdate.is_not_a_date_and_time())
	{
		std::cerr << "Calibration file does not exist!\n";
		return false;
	}
	else
	{
		boost::posix_time::ptime current(boost::posix_time::second_clock::local_time());
		boost::posix_time::time_duration lapse(current - lastUpdate);

		if (lapse.total_seconds() > calibUpdate)
		{
			std::cout << "Calibration out of date!\n";
			return false;
		}
		else
			return true;
	}
}
