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
			if (m_data->isCalibrated)
				pureTrace = GetPureTrace();
			break;
		case state::calibration:
			if (m_data->gdconc == 0.0)
				pureTrace = AverageTrace(1);
			else if (pureTrace.size())
				CalibrationTrace(m_data->gdconc, m_data->gd_err);
			else
				CalibrationTrace(-1.0, -1.0);
		case state::calibration_done:
			Calibration();
			LinearFit();
			break;
		case state::measurement:
			MeasurementTrace();
			break;
		case state::measurement_done:
			BulbCheck();
		case state::finalise:
			Finalise();
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

std::vector<double> Analysis::PureTrace()
{
	GdTree *cal = m_data->currentTree;
	std::vector<double> 
	for (int i = 0; i < cal->GetEntries(); ++i)
	{
		cal->GetEntry(i);
		if (cal->gdconc == 0)

}

std::vector<double> Analysis::AverageTrace(bool darkRemove)
{
	int size = vTraceCollect.front().size();
	std::vector<double> avgTrace(2 * size);
	if (!size)
		std::cout << "Houston, we have a problem\n";
	else
	{
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
			      avgTrace[e]  / pow(avgTrace[i]   * log(10), 2);

		absTrace[i] = log10(pureTrace[i] / avgTrace[i]); 
	}

	return absTrace;
}

std::vector<double> Analysis::Absorbance(const std::vector<double> &avgTrace, int &i1, int &i2)
{
	std::vector<double> absTrace = AbsorbTrace(avgTrace);

	std::vector<int> iPeak, iDeep;
	FindPeakDeep(absTrace, iPeak, iDeep);

	if (iPeak.size() > 0)
		i1 = iPeak.at(0);
	if (iPeak.size() > 1)
		i1 = iPeak.at(1);

	return absTrace;
}

void Analysis::CalibrationTrace(double gdconc, double gd_err)
{
	/* fill calibration tree
	 * if there is no averaged trace, there is no point to this
	 */
	std::vector<double> avgTrace = AverageTrace(1);
	if (!avgTrace.size())
		return;

	int size = avgTrace.size() / 2;
	GdTree *tree = m_data->currentTree;

	int i1 = -1, i2 = -1;
	std::vector<double> absTrace = Absorbance(avgTrace, i1, i2);

	tree->GdConc = gdconc;
	tree->Gd_Err = gd_err;

	tree->Trace = std::vector<double>(avgTrace.begin(), avgTrace.begin() + size);
	tree->T_Err = std::vector<double>(avgTrace.begin() + size, avgTrace.end());

	/* if there is a puretrace then continue doing calibration and filling the tree
	 * if not, fill the tree partially and complete it later, once the pure water trace
	 * has been taken (hopefully before the end of the calibration
	 * a few entries are filled with empty vectors or negative numbers which
	 * denotes a incomplete calibration
	 */
	if (pureTrace.size())
	{
		std::vector<double> absTrace = Absorbance(avgTrace, i1, i2);
		tree->Absor = std::vector<double>(absTrace.begin(), absTrace.begin() + size);
		tree->A_Err = std::vector<double>(absTrace.begin() + size, absTrace.end());
	}
	else
	{
		tree->Absor = std::vector<double>();
		tree->A_Err = std::vector<double>();
	}

	if (i1 >= 0)
	{
		tree->Wavelength_1 = X.at(i1);
		tree->Absorbance_1 = absTrace.at(i1);
		tree->Absorb_Err_1 = absTrace.at(i1 + size);

		tree->Absorb_Diff = tree->Absorbance_1;
		tree->AbsDiff_Err = tree->Absorbance_1;
	}
	else
	{
		tree->Wavelength_1 = -1.0;
		tree->Absorbance_1 = -1.0;
		tree->Absorb_Err_1 = -1.0;

		tree->Absorb_Diff = 0.0;
		tree->AbsDiff_Err = 0.0;
	}

	if (i2 >= 0)
	{
		tree->Wavelength_2 = X.at(i2);
		tree->Absorbance_2 = absTrace.at(i2);
		tree->Absorb_Err_2 = absTrace.at(i2 + size);

		tree->Absorb_Diff -= tree->Absorbance_2;
		tree->AbsDiff_Err += tree->Absorbance_2;
	}
	else
	{
		tree->Wavelength_2 = -1.0;
		tree->Absorbance_2 = -1.0;
		tree->Absorb_Err_2 = -1.0;

		tree->Absorb_Diff = 0.0;
		tree->AbsDiff_Err = 0.0;
	}

	tree->Fill();
}

void Analysis::CalibrationComplete()
{
	if (pureTrace.size())	//this is bad
		return;

	GdTree *oldTree = m_data->currentTree;	//should be calibration tree
	GdTree *newTree = new GdTree(*oldTree);

	for (int i = 0; i < tree->GetEntries(); ++i)
	{
		oldTree->GetEntry(i);

		newTree->GdConc = oldTree->GdConc;
		newTree->Gd_Err = oldTree->Gd_Err;

		newTree->Trace = oldTree->Trace;
		newTree->T_Err = oldTree->T_Err;

		if (oldTree->Wavelength_1 < 0 || oldTree->Wavelength_2 < 0)
		{
			int size = oldTree->Trace.size();
			std::vector<double> avgTrace = oldTree->Trace;
			avgTrace.insert(avgTrace.end(), oldTree->T_Err.begin(), oldTree->T_Err.end());

			int i1 = -1.0, i2 = -1.0;
			std::vector<double> absTrace = Absorbance(avgTrace, i1, i2);

			newTree->Absor = std::vector<double>(absTrace.begin(), absTrace.begin() + size);
			newTree->A_Err = std::vector<double>(absTrace.begin() + size, absTrace.end());

			newTree->Wavelength_1 = m_data->xAxis.at(i1);
			newTree->Absorbance_1 = absTrace.at(i1);
			newTree->Absorb_Err_1 = absTrace.at(i1 + size);

			newTree->Wavelength_2 = X.at(i2);
			newTree->Absorbance_2 = absTrace.at(i2);
			newTree->Absorb_Err_2 = absTrace.at(i2 + size);

			newTree->Absorb_Diff = oldTree->Absorbance_1 - oldTree->Absorbance_2;
			newTree->AbsDiff_Err = oldTree->Absorb_Err_1 + oldTree->Absorb_Err_2;
		}
		else
		{
			newTree->Absor = oldTree->Absor;
			newTree->A_Err = oldTree->A_Err;

			newTree->Wavelength_1 = oldTree->Wavelength_1
			newTree->Absorbance_1 = oldTree->Absorbance_1
			newTree->Absorb_Err_1 = oldTree->Absorb_Err_1

			newTree->Wavelength_2 = oldTree->Wavelength_2
			newTree->Absorbance_2 = oldTree->Absorbance_2
			newTree->Absorb_Err_2 = oldTree->Absorb_Err_2

			newTree->Absorb_Diff = oldTree->Absorb_Diff;
			newTree->AbsDiff_Err = oldTree->AbsDiff_Err;
		}

		newTree->Fill();
	}

	delete oldTree;
	m_data->currenTree = newTree;
}


void Analysis::MeasurementTrace()
{
	std::vector<double> avgTrace, absTrace;
	int i1, i2;

	if (Absorbance(avgTrace, absTrace, i1, i2))
	{
		int size = avgTrace.size() / 2;
		GdTree *tree = m_data->currentTree;

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
	/* calibration tree will contain n+1 entries,
	 * where n is the number of concentration probed
	 * and +1 is the pure water reference
	 */
	GdTree *tree = m_data->currentTree;
	int n = tree->GetEntries();

	/* creating a graph with calibration points
	 * to be fitted with a+b*x
	 */
	TGraphErrors *gd = new TGraphErrors(n-1);
	int i = 0, j = 0;
	while (i < n)
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

	/* creat function a + b*x for fit the graph
	 */
	TF1 *line = new TF1("dep", "1 ++ x", 0, 1);
	int stat = gd->Fit(line, "RNQ");

	if (stat >= 0)	//successful fit
	{
		double a_val = gd->GetParameter(0);	//[0]
		double b_val = gd->GetParameter(1);	//[1]
		double a_err = gd->GetParError(0);	//[2]
		double b_err = gd->GetParError(1);	//[3]

		/* invert function line to have
		 * concentration as a function of absorbance x = (y-a)/b
		 */
		TF1 *ff = new TF1("gdconc", "(x - [0])/[1]", xmin, xmax);						//main inverse function
		TF2 *ee = new TF1("gd_err", "gdconc(x)^2 * ( (y^2 + [2]^2)/(x - [0])^2 + ([3]/[1])^2 )", xmin, xmax);	//propagation of uncertainty

		ff->SetParameter(0, a_val);
		ff->SetParameter(1, b_val);

		ee->SetParameter(0, a_val);
		ee->SetParameter(1, b_val);
		ee->SetParameter(2, a_err);
		ee->SetParameter(3, b_err);

		/* store functions in data model, to save them
		 */
		m_data->concentrationFunction = ff;
		m_data->concentrationFunc_Err = ee;
	}

	delete gd;
}

/* this routine can be improved
 * it is not robust and stable
 * but it works quite ok for now
 */
void Analysis::FindPeakDeep(const std::vector<double> &vTrace, std::vector<unsigned int> &iPeak, std::vector<unsigned int> &iDeep)
{
	/* Find maximum (and min) value and position
	 * this is needed to find the peaks of a trace
	 */
	int size = vTrace.size() / 2;
	double fMax = vTrace.front(), fMin = vTrace.front();;
	int iMax = 0, iMin = 0;
	for (int i = 1; i < size; ++i)
	{
		if (vTrace[i] > fMax)
		{
			fMax = vTrace[i];
			iMax = i;
		}
		if (vTrace[i] < fMin)
		{
			fMin = vTrace[i];
			iMin = i;
		}
	}

	/* a more robust defintion of threshold must be found
	 */
	double thr = 0.05 * (fMax - fMin);

	std::deque<unsigned int> dPeak, dDeep;
	dPeak.push_back(iMax);

	/* start peak searching from highest peak
	 * going left first, and then right
	 */ 
	for (int Dir = -1; Dir < 2; Dir += 2)
	{
		double PoV = false;		//F looking for Deep, T looking for P
		int iD = iMax, iS = iMax;
		double fS = fMax;
		/* if the last poinst found was a peak,
		 * the the algorithm searches for a deep
		 * and viceversa
		 */
		while (iD > iA && iD < iB)
		{
			int Sign = 2*PoV - 1;	//-1 looking for Deep, +1 looking for Peak
			if ( Sign*(vTrace.at(iD) - fS) > thr)
			{
				iS = iD;
				fS = vTrace.at(iD);
			}

			if (-Sign*(vTrace.at(iD) - fS) > thr)
			{
				double fZ = -Sign*fMax;
				int    iZ = -1;
				for (int j = iD; Dir*(iS-j) < 1 && j > -1 && j < vTrace.size(); j -= Dir)
				{
					if (Sign*(vTrace.at(j)-fZ) > 0)
					{
						fZ = vTrace.at(j);
						iZ = j;
					}
				}
				if (iZ > -1)
				{
					std::deque<unsigned int> &dRef = PoV ? dPeak : dDeep;

					if (Dir < 0)
						dRef.push_front(iZ);
					else
						dRef.push_back(iZ);

					fS = fZ;
					iD = iS = iZ;
					PoV = !PoV;
				}
			}
			iD += Dir;
		}
	}

	std::sort(dPeak.begin(), dPeak.end(), Sort(vTrace,  1));
	std::sort(dDeep.begin(), dDeep.end(), Sort(vTrace, -1));

	iPeak.clear();
	iPeak.insert(iPeak.end(), dPeak.begin(), dPeak.end());
	iDeep.clear();
	iDeep.insert(iDeep.end(), dDeep.begin(), dDeep.end());
}
