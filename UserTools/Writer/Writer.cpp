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
	Loop();

	switch (m_data->mode)
	{
		case state::init:
			break;
		case state::calibration_done:
			WriteCalibration();
			break;
		case state::measurement_done:
			WriteMeasurement();
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

/* this method saves calibration trees in the calibration output
 * it is arranged in sub-directories with the same name as the calibration + timestamp
 * the sub-directories will contain the calibration and the fitting functions, if present
 * the method creates the subdir, unless they exist already
 */
void Writer::WriteCalibration()
{
	//	    012345678901234
	//format is YYYYMMDDTHHMMSS (there shouldn't be fractional seconds
	boost::posix_time::ptime current(boost::posix_time::second_clock::local_time());
	std::string tc = boost::posix_time::to_iso_string(current);

	TFile file(m_data->calibrationFile.c_str(), "UPDATE");

	std::map<std::string, GdTree*>::iterator it = m_data->m_gdtrees.begin();
	for (; it != m_data->m_gdtrees.end(); ++it)
	{
		if (it->first.find(m_data->calibrationBase) != 0)	//string starts with base_name
			continue;

		std::string dirname = it->first + "_" + tc;

		if (!file.GetDirectory(dirname.c_str()))	//no sub-dir
			file.mkdir(dirname.c_str());

		file.cd(dirname.c_str());
		it->second->Write();

		if ((it->first == m_data->concentrationName) &&
				m_data->concentrationFunction &&
				m_data->concentrationFunc_Err)
		{
			m_data->concentrationFunction->Write();
			m_data->concentrationFunc_Err->Write();
		}
		if (it->first.find(m_data->calibrationBase) == 0)
			it->second->Write();
	}

	file.Close();
}

/* this method saves measurement trees in the measurment output
 * it is arranged in sub-directories with the same name as the measurement
 * the sub-directories will contain also the calibration used for that measurement
 * the method creates the subdir, unless they exist already
 */
void Writer::WriteMeasurement()
{
	TFile file(m_data->measurementFile.c_str(), "UPDATE");

	std::map<std::string, GdTree*>::iterator it = m_data->m_gdtrees.begin();
	for (; it != m_data->m_gdtrees.end(); ++it)
	{
		std::string dirname = it->first;

		if (dirname.find(m_data->calibrationBase) == 0)
			dirname.replace(0, m_data->calibrationBase.length(), m_data->measurementBase);

		if (!file.GetDirectory(dirname.c_str()))	//no sub-dir
			file.mkdir(dirname.c_str());

		file.cd(dirname.c_str());
		it->second->Write();
	}

	file.Close();
}

void Writer::Loop()
{
	std::cout << "Currently in GdTree map: " << m_data->m_gdtrees.size() << std::endl;
	std::map<std::string, GdTree*>::iterator it = m_data->m_gdtrees.begin();
	for (; it != m_data->m_gdtrees.end(); ++it)
	{
		std::cout << "  ---   " << it->first << ",\t" << it->second;
		if (it->second)
			std::cout << ":\t" << it->second->GetEntries() << std::endl;
		else
			std::cout << std::endl;
	}
}
