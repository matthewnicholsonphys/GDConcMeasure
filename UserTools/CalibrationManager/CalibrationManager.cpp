#include "CalibrationManager.h"

CalibrationManager::CalibrationManager():Tool(){}


bool CalibrationManager::Initialise(std::string configfile, DataModel &data)
{
	if (configfile != "")
	{
		m_configfile = configfile;
		m_variables.Initialise(configfile);
	}
	//m_variables.Print();

	m_data = &data;

	Configure();
	return true;
}


bool CalibrationManager::Execute()
{
	switch (m_data->mode)
	{
		case state::init:	//about to make measurement, check LED mapping
			Initialise(m_configfile, *data);
			if (IsCalibrated())
			{
				LoadCalibration();
				m_data->isCalibrated = true;
				m_data->analyseTree = calibName;
			}
			else
			{
				NewCalibration();
				m_data->isCalibrated = false;
			}
			break;
		case state::calibration:		//turn on led set
			std::string input;
			/* need some other smart way of doing this!
			 */
			//std::cout << "Concentration loaded (0 for pure water, any other key to quit): ";
			//std::cin >> input;
			if (!std::isdigit(input[0]))	//quit command occured
			{
				m_data->calibrationDone = true;
				std::cout << "Calibration complete\n";
			}
			else
			{
				m_data->calibrationDone = false;

				m_data->ledON = m_data->LED_name[calibLED];	//turn on LED UV

				m_data->gdconc = std::strtod(input, NULL);	//passing this to analysis
				m_data->gd_err = 0.0;

				m_data->analyseTree = calibName;
				//if (gd == 0)
				//else
				//	m_data->mode = state::calibrate_gd;
			}
			break;
		case state::calibration_done:
			m_data->currentTree = m_data->GetGdTree(treeName);
			m_data->isCalibrated = true;
			break;
		case state::finalise:
			//delete some tree/function objects?
			break;
		default:
			break;
	}

	return true;
}

bool CalibrationManager::Finalise()
{
	return true;
}

void CalibrationManager::Configure()
{
	m_variables.Get("verbose", verbose);

	m_variables.Get("calibration",	calibFile);	//file in which calibration is saved
	m_variables.Get("tree_name",	treeName);	//name of calibration tree
	m_variables.Get("concfunction",	concFunc);	//name of concentration function
	m_variables.Get("err_function",	err_Func);	//name of uncertainity function

	m_variables.Get("update",	updateTime);	//number of seconds every time calibration should happen
	m_variables.Get("routine",	fileList);	//file in which calibration is saved
	m_variables.Get("LED_name",	calibLED);	//name of LED used to do calibration
}

void CalibrationManager::LoadCalibration()
{
	TFile inFile(calibFile, "OPEN");
	TF1 * valF = static_cast<TF1*>(inFile.Get(concFunc)->Clone());
	TF2 * errF = static_cast<TF2*>(inFile.Get(err_Func)->Clone());

	valF->SetDirectory(0);
	errF->SetDirectory(0);

	inFile.Close();

	GdTree *calib = new GdTree(treeName, calibFile);

	m_data->AddGdTree(treeName, calib);
	m_data->concentrationFunction = valF;
	m_data->concentrationFunc_Err = errF;
}

void CalibrationManager::NewCalibration()
{
	m_data->DeleteGdTree(treeName);

	delete m_data->concentrationFunction;
	delete m_data->concentrationFunc_Err;
	m_data->concentrationFunction = 0;
	m_data->concentrationFunc_Err = 0;

	GdTree *calib = new GdTree(treeName);
	m_data->AddGdTree(treeName, calib);
}

bool CalibrationManager::IsCalibrated()
{
	std::string calibFile;
	int calibUpdate;

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
