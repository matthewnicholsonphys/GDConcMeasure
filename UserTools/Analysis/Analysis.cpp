#include "Analysis.h"

Analysis::Analysis():Tool(){}


bool Analysis::Initialise(std::string configfile, DataModel &data){

  if(configfile!="")  m_variables.Initialise(configfile);
  //m_variables.Print();

  m_data= &data;

  return true;
}


bool Analysis::Execute()
{	
	//analyse means data was loaded in datamodel
	//and trace can be averaged
	switch (m_data->mode)
	{
		case state::init:
			darkTrace.clear();
			pureTrace.clear();
			vTraceCollect.clear();
			break;
		case state::measure_start:
			LoadTraces(m_data->filePath);
			MeasurementTrace();
			break;
		case state::calibrate_dark:
			LoadTraces(m_data->filePath);
			darkTrace = AverageTrace(0);
			break;
		case state::calibrate_pure:
			LoadTraces(m_data->filePath);
			pureTrace = AverageTrace(1);	//and remove dark
		case state::calibrate_gd:
			LoadTraces(m_data->filePath);
			CalibrationTrace(m_data->gd, m_data->gd_err);
			break;
		case state::measure_stop:
			m_data->mode = state::write_measurement;
			break;
		case state::calibrate_fit:
			FitAbsorbance();
			m_data->mode = state::write_calibration;
			break;
		default:
			break;
	}

	return true;
}

bool Analysis::Finalise()
{
	darkTrace.clear();
	pureTrace.clear();
	return true;
}

void Analysis::LoadTraces(const std::string &filePath)
{
	vTraceCollect.clear();

	std::string line = std::string ("ls ") + filePath + " > .tmp_filelist";
	system(line.c_str());

	std::vector<std::string> fileList;
	std::vector<std::string>::iterator il;

	std::ifstream in(".tmp_filelist");
	while (std::getline(inFiles, line))
		fileList.push_back(line);
	in.close();

	for (il = fileList.begin(); il != fileList.end(); ++il)
	{
		bool fillX = !xAxis.size()
		std::vector<double> intensity;
		in.open(il->c_str());
		while (std::getline(in, line))
		{
			std::stringstream ssl(line);
			ssl >> x >> y;

			if (x >= xMin && x <= xMax)
			{
				if (fillX)
					xAxis.push_back(x);

				intensity.push_back(y);
			}
		}

		vTraceCollect.push_back(intensity);
	}
}

std::vector<double> Analysis::AverageTrace(bool darkRemove)
{
	int size = vTraceCollect.front().size();
	std::vector<double> avgTrace(size * 2);
	std::vector<std::vector<double> >::iterator it;
	std::vector<double>::iterator il;
	for (it = vTraceCollect.begin(); it != vTraceCollect.end(); ++it)
	{
		int l = 0;
		for (il = it->begin(); il != it->end(); ++il, ++l)	//loop "size" entries
		{
			avgTrace[l]	      += *il / size;
			avgTrace[l + size] += pow(*il, 2) / size / (size - 1);
		}
	}

	std::vector<double> &subTrace =
			(darkRemove && darkTrace.size()) ? darkTrace : std::vector<double>(size);
	for (int i = 0; i < avgTrace.end(); ++l)
	{
		avgTrace[l + size] -= pow(avgTrace[l], 2) / (size - 1);

		//if there is darkRemove, subTrace is equal to darkTrace
		//otherwise it is just empty
		avgTrace[l]	   -= subTrace[l];
		avgTrace[l + size] += subTrace[l + size];
	}

	return avgTrace;
}

bool Analysis::Absorbance(std::vector<double> &avgTrace, std::vector<double> &absTrace,
		int &i1, int &i2)
{
	std::vector<double> avgTrace = AverageTrace(1);
	std::vector<double> absTrace = AbsorbTrace(avgTrace);

	std::vector<int> iPeak, iDeep;
	FindPeakValley(absTrace, iPeak, iDeep, 1);

	if (iPeak.size() > 1)
	{
		i1 = iPeak.at(0);
		i2 = iPeak.at(1);
		return true;
	}
	else
		return false;
}

void Analysis::CalibrationTrace(double gdconc, double gd_err)
{
	std::vector<double> avgTrace, absTrace;
	int i1, i2;
	if (Absorbance(avgTrace, absTrace, i1, i2))
	{
		int size = avgTrace.size() / 2;
		GdConc *calib = m_data->GetTree("calib");

		tree->Gd_Conc = gdconc;
		tree->Gd_Conc_Err = gd_err;
		tree->Wavelength_1 = X.at(i1);
		tree->Absorbance_1 = absTrace.at(i1);
		tree->Absorb_Err_1 = absTrace.at(i1 + size);
		tree->Wavelength_2 = X.at(i2);
		tree->Absorbance_2 = absTrace.at(i2);
		tree->Absorb_Err_2 = absTrace.at(i2 + size);

		tree->AbsorbDiff  = tree->Absorbance_1 - tree->Absorbance_2;
		tree->AbsDiff_Err = tree->Absorbance_1 + tree->Absorbance_2;

		tree->nTrace	= size;
		tree->Trace	= &avgTrace[0];
		tree->Trace_Err = &avgTrace[size];

		tree->Fill();
	}
}

void Analysis::MeasurementTrace()
{
	std::vector<double> avgTrace, absTrace;
	int i1, i2;
	if (Absorbance(avgTrace, absTrace, i1, i2))
	{
		int size = avgTrace.size() / 2;
		GdConc *calib = m_data->GetTree("calib");

		tree->Wavelength_1 = X.at(i1);
		tree->Absorbance_1 = absTrace.at(i1);
		tree->Absorb_Err_1 = absTrace.at(i1 + size);
		tree->Wavelength_2 = X.at(i2);
		tree->Absorbance_2 = absTrace.at(i2);
		tree->Absorb_Err_2 = absTrace.at(i2 + size);

		tree->AbsorbDiff  = tree->Absorbance_1 - tree->Absorbance_2;
		tree->AbsDiff_Err = tree->Absorbance_1 + tree->Absorbance_2;

		tree->nTrace	= size;
		tree->Trace	= &avgTrace[0];
		tree->Trace_Err = &avgTrace[size];

		double gd_con = func->Eval(tree->AbsorbDiff);
		double gd_err = erro->Eval(tree->AbsorbDiff, tree->AbsDiff_Err);

		tree->Gd_Conc	  = gd_con;
		tree->Gd_Conc_err = gd_err;

		tree->Fill();
	}
}
