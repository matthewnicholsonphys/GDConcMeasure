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

	m_variables.Get("verbose", verbose);

	m_variables.Get("calibration",	calibFile);	//file with list of calibration/measurement
	m_variables.Get("output",	outputFile);	//file in which calibration is saved
	m_variables.Get("base_name",	base_name);	//base_name for calibation
	m_variables.Get("concfunction",	concFuncName);	//name of concentration function
	m_variables.Get("err_function",	err_FuncName);	//name of uncertainity function

	return true;
}


bool CalibrationManager::Execute()
{
	switch (m_data->mode)
	{
		case state::init:	//about to make measurement, check LED mapping
			Initialise(m_configfile, *m_data);
			calibList = LoadCalibration(updateList);
			if (updateList.size())
			{
				//calibration out of date!
				m_data->isCalibrated = false;
				NewCalibration();
				ic = updateList.begin();	//global iterator
			}
			else
				m_data->isCalibrated = true;
			break;
		case state::calibration:		//turn on led set
			if (ic != calibList.end())
			{
				Calibrate();		//no clue how to do this
				++ic;
			}
			else
			{
				m_data->calibrationDone = true;
				std::cout << "Calibration completed\n";
			}
			break;
		case state::calibration_done:
			m_data->isCalibrated = true;
			break;
		case state::finalise:
			Finalise();
			//delete some tree/function objects?
			break;
		default:
			break;
	}

	return true;
}

bool CalibrationManager::Finalise()
{
	//clean
	if (m_data->calibrationFile && m_data->calibrationFile->IsOpen())
	{
		m_data->calibrationFile->Close();
		m_data->calibrationFile = NULL;
	}

	for (ic = calibList.begin(); ic != calibList.end(); ++ic)
		m_data->DeleteGdTree(*ic);

	calibList.clear();
	updateList.clear();

	delete m_data->concentrationFunction;
	delete m_data->concentrationFunc_Err;
	m_data->concentrationFunction = 0;
	m_data->concentrationFunc_Err = 0;

	return true;
}

//get list of calibration needed
//eg.
//*led0		time1000		#measurement of led0, to be calibrated every 1000 days.
//					#The star indicates this is the concentration measurement
//led0, led1, led2, time54321		#measurement of led0_led1_led2, to be calibrated every 54321 days
//led1 led3, 	time99999		#measurement of led1_led3, to be calibrated every 99999 days
//
std::vector<std::string> CalibrationManager::CalibrationList()
{
	std::vector<std::string> vList;

	std::ifstream cf(calibFile.c_str());
	std::string line;
	while (std::getline(cf, line))
	{
		if (line.find_first_of('#') != std::string::npos)
			line.erase(line.find_first_of('#'));

		bool ct;
		if (line.find_first_of('*') != std::string::npos)
			ct = true;
		else
			ct = false;
		
		std::stringstream ssl(line);
		std::string name, word;
		int tu;
		while (ssl)
		{
			if (!std::isalnum(ssl.peek()) && ssl.peek() != '_')
				ssl.ignore();
			else if (ssl >> word)
			{
				if (word.find("time") != std::string::npos)
					tu = std::strtol(word.substr(word.find("time")+1).c_str(), NULL, 10);
				else
					name += word + "_";
			}
		}

		name.erase(name.size()-1);
		vList.push_back(name);

		timeUpdate[name] = tu;

		if (ct)
			concTreeName = name;
	}

	return vList;
}

/* this routine will load all the calibration listed in calibFile
 * and the trees are saved in the data model
 * returns a list of calibrations to do
 */
std::vector<std::string> CalibrationManager::LoadCalibration(std::vector<std::string> &uList)
{
	//get list of calibrations
	std::vector<std::string> cList = CalibrationList();

	//open read only calibration file
	m_data->calibrationFile = new TFile(outputFile.c_str(), "OPEN");
	if (m_data->calibrationFile->IsZombie())
	{
		std::cerr << "Calibration file does not exist!\n";
		uList = cList;
		return cList;
	}

	uList.clear();

	//find latest calibration of each type
	TList *lk = m_data->calibrationFile->GetListOfKeys();
	lk->Sort(kSortDescending);

	//loop on calibration directories
	//they are sorted from last to first
	//directory names are "calibname_timestamp"
	//they contain a GdTree named "calib" and possily functions
	std::string type;
	for (int l = 0; l < lk->GetEntries(); ++l)
	{
		//skip forward because this type is already done
		std::string name = lk->At(l)->GetName();
		if (!type.empty() && name.find(type) != std::string::npos)
			continue;

		//loop on list of calibration types, which are "calibname"
		for (ic = cList.begin(); ic != cList.end(); ++ic)
		{
			//name match to find it
			if (name.find(*ic) != std::string::npos)
			{
				type = *ic;	//store type, needed to fast forward loop

				if (IsUpdate(name, timeUpdate[name]))	//needs an update
				{
					Load(name, type);
					uList.push_back(type);
				}

				//found it, so break
				break;
			}
		}
	}

	return cList;
}

void CalibrationManager::Load(std::string name, std::string type)
{
	std::string gdname = name + "/" + type;
	TTree *gdt = static_cast<TTree*>(m_data->calibrationFile->Get(gdname.c_str())->Clone());
	GdTree *calib = new GdTree(gdt);
	std::string cname = base_name + "_" + type;
	m_data->AddGdTree(cname, calib);

	if (type == concTreeName)	//happens once
	{
		m_data->concentrationName = cname;

		std::string f1name = name + "/" + concFuncName;
		std::string f2name = name + "/" + err_FuncName;

		//clone tree and functions
		TF1 * valF = static_cast<TF1*>(m_data->calibrationFile->Get(f1name.c_str())->Clone());
		TF2 * errF = static_cast<TF2*>(m_data->calibrationFile->Get(f2name.c_str())->Clone());

		m_data->concentrationFunction = valF;
		m_data->concentrationFunc_Err = errF;
	}

}

//directory names are "calibname_timestamp"
//with timestamp an iso string YYYYMMDDThhmmss
bool CalibrationManager::IsUpdate(std::string name, int timeUpdate)
{
	std::string ts = name.substr(name.find_last_of('_') + 1);

	//get current time
	boost::posix_time::ptime last(boost::posix_time::from_iso_string(ts));
	boost::posix_time::ptime current(boost::posix_time::second_clock::local_time());
	boost::posix_time::time_duration lapse(current - last);

	if (lapse.hours() > timeUpdate)
	{
		std::cout << "Calibration out of date!\n";
		return false;
	}
	else
		return true;
}

void CalibrationManager::NewCalibration()
{
	if (m_data->calibrationFile)
		m_data->calibrationFile->Close();

	m_data->calibrationFile = new TFile(outputFile.c_str(), "UPDATE");

	for (ic = updateList.begin(); ic != updateList.end(); ++ic)
	{
		std::string name = base_name + "_" + *ic;
		GdTree *calib = new GdTree(name.c_str());
		m_data->AddGdTree(name.c_str(), calib);

		if (*ic == concTreeName)	//happens once
		{
			m_data->concentrationName = name;

			//create functions for absorbance, value and error
			m_data->concentrationFunction = new TF1(concFuncName.c_str(),
					"(x - [0])/[1]", 0, 1);
			m_data->concentrationFunc_Err = new TF2(err_FuncName.c_str(),
					"((x - [0])/[1])^2 * ( (y^2 + [2]^2)/(x - [0])^2 + ([3]/[1])^2 )", 0, 1);
		}
	}
}

//calibrate is like a measurement
bool CalibrationManager::Calibrate()
{
	std::string acknowledge;	// <--- this is some form of aknoledgment
	if (*ic != concTreeName)
	{
		std::cout << "have you put pure water? Push button when done\n";
		//acknowledge!!
	}
	else
	{
		std::cout << "which concentration have you put " << *ic << "?\n";
		//acknowledge!!

		m_data->gdconc = std::strtod(acknowledge.c_str(), NULL);
		m_data->gd_err = std::strtod(acknowledge.c_str(), NULL);
	}

	m_data->turnOnPump = true;	//must change water
	m_data->turnOnLED = *ic;
	m_data->calibrationName = base_name + "_" + *ic;
}
