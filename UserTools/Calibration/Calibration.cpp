#include "Calibration.h"

Calibration::Calibration():Tool(){}


bool Calibration::Initialise(std::string configfile, DataModel &data){

	if(configfile!="")  m_variables.Initialise(configfile);
	//m_variables.Print();

	m_data= &data;

	return true;
}


bool Calibration::Execute(){

	return true;
}


bool Calibration::Finalise(){

	return true;
}


bool Calibration::isCalibrated()
{
	std::string calibFile;
	int calibUpdate;
	m_variables.Get("calibration", calibFile);	//file in which calibration is saved
	m_variables.Get("update",      calibUpdate);	//number of seconds every time calibration should happen

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
		m_data->isCalibrated = false;
	}
	else
	{
		boost::posix_time::ptime current(boost::posix_time::second_clock::local_time());
		boost::posix_time::time_duration lapse(current - lastUpdate);

		if (lapse.total_seconds() > calibUpdate)
		{
			std::cout << "Calibration out of date!\n";
			m_data->isCalibrated = false;
		}
		else
			m_data->isCalibrated = false;
	}
}

