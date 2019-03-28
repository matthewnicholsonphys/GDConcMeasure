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
				CalibrationTrace(m_data->treeName, m_data->gdconc, m_data->gd_err);
		case state::calibration_done:
			LinearFit();
			break;
		case state::measurement:
			MeasurementTrace(m_data->treeName);
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
	FindPeakDeep(absTrace, iPeak, iDeep);

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

void TrackFunctions::Analyse(const std::vector<double> &vTrace)
{
	int size = vTrace.size() / 2;
	iA = 0;
	iB = size;

	fArea = 0.0;
	fA1  = 0.0;
	fA2  = 0.0;
	fAreaVar = 0.0;
	fA1Var   = 0.0;	//-1 if no error
	fA2Var   = 0.0;	//0 if error
	fT1 = (2*iA + iB)/3;
	fT2 = (iA + 2*iB)/3;

	fMax    = -100.0;
	fMin    =  100.0;
	fMaxVar = -1.0;
	fMinVar = -1.0;
	iMax    =  0.0;
	iMin    =  0.0;

	int Nf1 = 0, Nf2 = 2;
	for (int i = iA, e = iA + size; i < iB; ++i, ++e)
	{
		if (fMax < vTrack.at(i))
		{
			fMax = vTrack.at(i);
			iMax = i;
		}
		if (fMin > vTrack.at(i))
		{
			fMin = vTrack.at(i);
			iMin = i;
		}


		fArea += vTrack.at(j);
		if (Err)
			fAreaVar += vTrack.at(e);
		if (j < fT1)
		{
			fA1    += vTrack.at(j);
			if (Err)
				fA1Var += vTrack.at(e);
			++Nf1;
		}
		if (j > fT2)
		{
			fA2    += vTrack.at(j);
			if (Err)
				fA2Var += vTrack.at(e);
			++Nf2;
		}
	}

	//min and max error
	if (Err)
	{
		fMaxVar = vTrack.at(iMax+vTrack.size()/2);
		fMinVar = vTrack.at(iMin+vTrack.size()/2);
	}

	fThr = fPerc*(fMax-fMin);
}
void Analysis::FindPeakDeep(const std::vector<double> &vTrack, std::vector<unsigned int> &iPeak, std::vector<unsigned int> &iDeep)
{
	Analyse(vTrack);	//needed
	std::deque<unsigned int> dPeak, dDeep;

	dPeak.push_back(GetMax_i());

	//going left first, and then right
	for (int Dir = -1; Dir < 2; Dir += 2)
	{
		double PoV = false;		//F looking for Deep, T looking for P
		int iD = GetMax_i(), iS = GetMax_i();
		double fS = GetMax();
		while (iD > iA && iD < iB)
		{
			int Sign = 2*PoV - 1;	//-1 looking for Deep, +1 looking for Peak
			if ( Sign*(vTrack.at(iD) - fS) > GetThr())
			{
				iS = iD;
				fS = vTrack.at(iD);
			}

			if (-Sign*(vTrack.at(iD) - fS) > GetThr())
			{
				double fZ = -Sign*GetMax();
				int    iZ = -1;
				for (int j = iD; Dir*(iS-j) < 1 && j > -1 && j < vTrack.size(); j -= Dir)
				{
					if (Sign*(vTrack.at(j)-fZ) > 0)
					{
						fZ = vTrack.at(j);
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
	if (Sorting)
	{
		std::sort(dPeak.begin(), dPeak.end(), Sort(vTrack,  1));
		std::sort(dDeep.begin(), dDeep.end(), Sort(vTrack, -1));
	}

	iPeak.clear();
	iPeak.insert(iPeak.end(), dPeak.begin(), dPeak.end());
	iDeep.clear();
	iDeep.insert(iDeep.end(), dDeep.begin(), dDeep.end());
}

/*
void TrackFunctions::Smoothen(std::vector<double> &vTrack, unsigned int Integrate)
{
	SetTrack(vTrack);
	vTrack.swap(Smoothen(Integrate));
}
*/

void TrackFunctions::Smoothen(std::vector<double> &vTrack, unsigned int Integrate)
{
	for (int rep = 1; rep < Integrate; ++rep)
		for (int i = rep; i < vTrack.size()-rep; ++i)
			vTrack.at(i) = (vTrack.at(i+rep) + vTrack.at(i-rep))/2.0;
}

/*
void TrackFunctions::Markov(std::vector<double> &vTrack, int Window)
{
	SetTrack(vTrack);
	vTrack.swap(Markov(Window));
}
*/

void TrackFunctions::Markov(std::vector<double> &vTrack, int Window)
{
	Analyse(vTrack);
	std::vector<double> vMarkov(vTrack.size());
	vMarkov.front() = 1.0;
	double Norm = 1.0;
	for (int i = 0; i < vTrack.size()-1; ++i)
	{
		double sumF = 0, sumB = 0;
		double p0 = vTrack.at(i) / GetMax();
		double p1 = vTrack.at(i+1) / GetMax();
		for (int j = 1; j < Window; ++j)
		{
			double pF, ComptF;	//forward
			if (i+j > vTrack.size()-1)
				pF = vTrack.at(vTrack.size()-1);
			else
				 pF = vTrack.at(i+j)/GetMax();
			if(pF + p0 <= 0)
				ComptF = exp(pF - p0);
			else
				ComptF = exp((pF-p0)/sqrt(pF+p0));
			sumF += ComptF;

			double pB, ComptB;	//backward
			if (i+1-j < 0)
				pB = vTrack.at(0);
			else
				 pB = vTrack.at(i+1-j)/GetMax();
			if(pB + p1 <= 0)
				ComptB = exp(pB - p1);
			else
				ComptB = exp((pB - p1)/sqrt(pB+p1));
			sumB += ComptB;
		}

		vMarkov.at(i+1) = vMarkov.at(i) * sumF/sumB;
		Norm += vMarkov.at(i+1);
	}
	for (unsigned int i = 0; i < vTrack.size(); ++i)
		vMarkov.at(i) *= GetArea()/Norm;

	vTrack.swap(vMarkov);
}
