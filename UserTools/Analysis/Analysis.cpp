#include "Analysis.h"

Analysis::Analysis():Tool(){}


bool Analysis::Initialise(std::string configfile, DataModel &data)
{

	if(configfile!="")
		m_variables.Initialise(configfile);
	//m_variables.Print();

	m_data = &data;

	return true;
}


bool Analysis::Execute()
{	
	//analyse means data was loaded in datamodel
	//and trace can be averaged
	switch (m_data->mode)
	{
		case state::init:
			darkTrace = AverageTrace(0);
			break;
		case state::calibration:
			if (m_data->gdconc == 0.0)
				pureTrace = AverageTrace(1);
			else
				CalibrationTrace(m_data->calibrationName, m_data->gd, m_data->gd_err);
		case state::calibration_done:
			LinearFit();
			break;
		case state::measurement:
			MeasurementTrace(m_data->measurementName);
			break;
		case state::measurement_done:
			BulbCheck();
			pureTrace = AverageTrace(1);	//and remove dark
		case state::finalise:
			darkTrace.clear();
			pureTrace.clear();
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

/*
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
*/

std::vector<double> Analysis::AverageTrace(bool darkRemove)
{
	int size = vTraceCollect.front().size();
	std::vector<double> avgTrace(2 * size);

	std::vector<std::vector<double> >::iterator it;
	for (it = vTraceCollect.begin(); it != vTraceCollect.end(); ++it)
	{
		int l = 0;
		for (int i = 0, e = size; i < size; ++i, ++e)	//loop "size" entries
		{
			avgTrace[i] += (*it)[i] / size;
			avgTrace[e] += pow((*it)[i], 2) / size / (size - 1);
		}
	}

	std::vector<double> &subTrace =
			(darkRemove && darkTrace.size()) ? darkTrace : std::vector<double>(2 * size);

	for (int i = 0, e = size; i < size; ++i, ++e)
	{
		avgTrace[e] -= pow(avgTrace[i], 2) / (size - 1);

		//if there is darkRemove, subTrace is equal to darkTrace
		//otherwise it is just empty
		avgTrace[i] -= subTrace[i];
		avgTrace[e] += subTrace[e];
	}

	return avgTrace;
}

std::vector<double> Analysis::AbsorbTrace(const std::vector<double> &avgTrace)
{
	int size = avgTrace.size() / 2;
	std::vector<double> absTrace(size);

	for (int i = 0, e = size; i < size; ++i, ++e)
	{
		absTrace[e] = pureTrace[e] / pow(pureTrace[i]  * log(10), 2) +
			      avgTrace[e]  / pow(avgTrack[i]   * log(10), 2);

		absTrace[i] = log10(pureTrace[i] / avgTrace[i]); 
	}

	return absTrace;
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

void Analysis::CalibrationTrace(const std::string &name, double gdconc, double gd_err)
{
	GdTree *tree = m_data->GetTTree("calib");

	std::vector<double> avgTrace, absTrace;
	int i1, i2;
	if (Absorbance(avgTrace, absTrace, i1, i2))
	{
		int size = avgTrace.size() / 2;
		GdConc *calib = m_data->GetTree("calib");

		tree->GdConc = gdconc;
		tree->Gd_Err = gd_err;

		tree->Wavelength_1 = X.at(i1);
		tree->Absorbance_1 = absTrace.at(i1);
		tree->Absorb_Err_1 = absTrace.at(i1 + size);
		tree->Wavelength_2 = X.at(i2);
		tree->Absorbance_2 = absTrace.at(i2);
		tree->Absorb_Err_2 = absTrace.at(i2 + size);

		tree->Absorb_Diff = tree->Absorbance_1 - tree->Absorbance_2;
		tree->AbsDiff_Err = tree->Absorbance_1 + tree->Absorbance_2;

		tree->nTrace	= size;
		tree->Trace	= &avgTrace[0];
		tree->Trace_Err = &avgTrace[size];

		tree->Fill();
	}
}

void Analysis::MeasurementTrace(const std::string &name)
{
	GdTree *tree = m_data->GetTTree(name);

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

		tree->Absorb_Diff  = tree->Absorbance_1 - tree->Absorbance_2;
		tree->AbsDiff_Err = tree->Absorbance_1 + tree->Absorbance_2;

		tree->nTrace	= size;
		tree->Trace	= &avgTrace[0];
		tree->Trace_Err = &avgTrace[size];

		double gdconc = funcAbs->Eval(tree->Absorb_Diff);
		double gd_err = err_Abs->Eval(tree->Absorb_Diff, tree->AbsDiff_Err);

		tree->GdConc = gdconc;
		tree->Gd_Err = gd_err;

		tree->Fill();
	}
}

void Analysis::LinearFit()
{
	GdTree *tree = m_data->GetTTree("calib");
	int n = tree->GetEntries() - 1;	//first entry is pure water

	TGraphErrors *gd = new TGraphErrors(n);
	int i = 0, j = 0;
	while (i < n+1)
	{
		++i;
		tree->GetEntry(i);

		if (tree->GdConc > 0)
		{
			gd->SetPoint(j, tree->GdConc, tree->Absorb_Diff);
			gd->SetPointError(j, tree->Gd_Err, tree->Absorb_Diff);
			++j;
		}
	}

	//keeps point sorted in X
	gd->Sort();

	TF1 *func = new TF1("dep", "1 ++ x", 0, 1);

	int stat = gd->Fit(func, "RNQ");

	if (stat >= 0)
	{
		double a_val = gd->GetParameter(0);	//[0]
		double b_val = gd->GetParameter(1);	//[1]
		double a_err = gd->GetParError(0);	//[2]
		double b_err = gd->GetParError(1);	//[3]

		TF1 *ff = new TF1("gdconc", "(x - [0])/[1]", xmin, xmax);
		TF2 *ee = new TF1("gd_err", "gdconc(x)^2 * ( (y^2 + [2]^2)/(x - [0])^2 + ([3]/[1])^2 )", xmin, xmax);

		ff->SetParameter(0, a_val);
		ff->SetParameter(1, b_val);

		ee->SetParameter(0, a_val);
		ee->SetParameter(1, b_val);
		ee->SetParameter(2, a_err);
		ee->SetParameter(3, b_err);

		m_data->concentrationFunction = ff;
		m_data->concentrationFunc_Err = ee;
	}

	delete gd;
}
