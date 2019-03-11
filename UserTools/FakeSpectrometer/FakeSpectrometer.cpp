#include "FakeSpectrometer.h"

FakeSpectrometer::FakeSpectrometer():Tool(){}


bool FakeSpectrometer::Initialise(std::string configfile, DataModel &data){

  if(configfile!="")  m_variables.Initialise(configfile);
  //m_variables.Print();

  m_data= &data;

  mt = new TRandom3();

  noise = 0.0001;
  nnn = 500;
  conc = 0.2;
  pedestal = 1000*noise;

  return true;
}


bool FakeSpectrometer::Execute()
{
	switch (m_data->mode)
	{
		case state::init:	//wake up, connect spectrometer on USB
			//Initialise(m_configfile, *m_data);
			GetDark();	//measure dark noise on wake up
			break;
		case state::calibration:
			//if (m_data->gdconc > 0.0)
			//	GetData(m_data->gdconc);
			//else
			//	GetPure();
			break;
		case state::measurement:
			Wait();
			//GetData(conc);
			break;
		case state::finalise:
			//Finalise();
			break;
		default:
			break;
	}
	return true;
}


bool FakeSpectrometer::Finalise()
{
	return true;
}

void FakeSpectrometer::Wait()
{
	std::cout << "Waiting for you to finish... \n";
	std::cin.get();
}


bool FakeSpectrometer::GetDark()
{
	bool once = true;

	std::cout << "FS get dark" << std::endl;
	m_data->wavelength.clear();
	for (int i = 0; i < 1000; ++i)
	{
		m_data->wavelength.push_back(X(i));
		if (once)
			vx.push_back(X(i));
	}


	m_data->traceCollect.clear(); 
	for (int i = 0; i < nnn; ++i)
	{
		std::vector<double> tmp;
		for (int j = 0; j < 1000; ++j)
		{
			tmp.push_back(Dark(j));
			if (once)
				vdark.push_back(tmp.at(j));
		}
		once = false;

		m_data->traceCollect.push_back(tmp);
	}


	return true;
}

bool FakeSpectrometer::GetPure()
{
	std::cout << "FS get pure" << std::endl;
	bool once = true;

	m_data->traceCollect.clear(); 
	for (int i = 0; i < nnn; ++i)
	{
		std::vector<double> tmp;
		for (int j = 0; j < 1000; ++j)
		{
			tmp.push_back(Pure(j) + Dark(j));
			if (once)
				vpure.push_back(tmp.at(j));
		}
		once = false;

		m_data->traceCollect.push_back(tmp);
	}


	return true;
}

bool FakeSpectrometer::GetData(double conc)
{
	std::cout << "FS get data" << std::endl;
	bool once = true;

	m_data->traceCollect.clear(); 
	for (int i = 0; i < nnn; ++i)
	{
		std::vector<double> tmp;
		for (int j = 0; j < 1000; ++j)
		{
			tmp.push_back(Data(j, conc) + Dark(j));//-noise*10);
			if (once)
				vdata.push_back(tmp.at(j));
		}
		once = false;

		m_data->traceCollect.push_back(tmp);
	}


	return true;
}

double FakeSpectrometer::Pure(int i)
{
	return 0.1 * std::exp(- pow((X(i) - 400.0)/5.0, 2));
}

double FakeSpectrometer::Absorb(int i, double cc)
{
	return cc * 0.1  * std::exp(- pow((X(i) - 399.0)/0.5, 2)) + 
	       cc * 0.06 * std::exp(- pow((X(i) - 401.0)/0.5, 2));
}

double FakeSpectrometer::Data(int i, double cc)
{
	return Pure(i) - Absorb(i, cc);
}

double FakeSpectrometer::Dark(int i)
{
	return pedestal + mt->Uniform(-noise, noise);
	//return pedestal;
}

double FakeSpectrometer::X(int i)
{
	return 200.0 + 2.0 * i / 5.0;
}
