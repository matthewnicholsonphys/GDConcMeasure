#include "Spectrometer.h"

Spectrometer::Spectrometer() : Tool()
{
}

bool Spectrometer::Initialise(std::string configfile, DataModel &data)
{
	if(configfile!="")
		m_variables.Initialise(configfile);
	//m_variables.Print();

	m_data= &data;

	Configure();
	return true;
}


bool Spectrometer::Execute()
{
	switch (m_data->mode)
	{
		case state::init:	//wake up, connect spectrometer on USB
			EstablishUSB();
			DarkLevel();	//measure dark noise on wake up
			break;
		case state::calibration:
		case state::measurement:
			Measure();
			break;
		case state::finalise:
			CloseConnection();
			break;
		default:
			break;
	}

	return true;
}


bool Spectrometer::Finalise()
{
	return true;
}

void Spectrometer::Configure()
{
}

void Spectrometer::DarkLevel()
{
	/*
	 * take measurement with LED off
	 * this should be background to following measurements
	 * save in vTraceCollect all traces
	 * save in xAxis the wavelength of x-axis
	 */
}

void Spectrometer::Measure()
{
	/*
	 * take measurement with LED on
	 * save in vTraceCollect all traces
	 */
}
