#include "Scheduler.h"

Scheduler::Scheduler():Tool(){}

bool Scheduler::Initialise(std::string configfile, DataModel &data)
{
	if(configfile!="")
		m_variables.Initialise(configfile);
	//m_variables.Print();

	m_data = &data;

	m_data->mode = state::idle;

	m_variables.Get("verbose", verbose);

	m_variables.Get("idle",		idle_time);
	m_variables.Get("power_up",	power_up_time);
	m_variables.Get("power_down",	power_down_time);
	m_variables.Get("change_water", change_water_time);

	last = boost::posix_time::second_clock::local_time();

	stateName[state::idle]			= "idle";
	stateName[state::power_up]              = "power_up";
	stateName[state::power_down]            = "power_down";
	stateName[state::init]                  = "init";
	stateName[state::calibration]           = "calibration";
	stateName[state::calibration_done]      = "calibration_done";
	stateName[state::measurement]           = "measurement";
	stateName[state::measurement_done]      = "measurement_done";
	stateName[state::finalise]              = "finalise";
	stateName[state::turn_off_led]          = "turn_off_led";

	nextState = state::idle;
	rest_time = 0;
	
	//HACK
	//m_data->isCalibrated = true;

	return true;
}

bool Scheduler::Execute()
{
	std::cout << "mode start = " << stateName[m_data->mode] << std::endl;

	switch (m_data->mode)
	{
		case state::idle:
			last = Wait(rest_time);				//ToolDAQ is put to sleep until next measurement
			m_data->mode = state::power_up;			//time to power up the PSU!	in power up, Power tool turns on PSU
			break;
		case state::power_up:					//PSU has been turned on
			last = Wait(power_up_time);			//small wait to let PSU be stable
			m_data->mode = state::init;			//init the other Tools (calibration, measurement, LED, spectrometer, analysis...)
			break;
		case state::init:					//ToolChain has been configured, and dark noise has been taken and saved
			last = Wait();
			if (IsCalibrated())				//Check if calibration is present (was done by CalibrationManager during previous loop)
				m_data->mode = state::measurement;	//if there is calibration, then scheduler skips to measurement
			else
				m_data->mode = state::calibration;	//if there isn't calibration, this one must be done first!
			break;
		case state::calibration:
			last = Wait();
			if (IsCalibrationDone())				//check if calibration is completed
				nextState = state::calibration_done;	//if so, it can be saved to disk
			else
				nextState = state::calibration;	//if not, repeat calibration loop
			std::cout << "next state should be " << stateName[nextState] << std::endl;

			if (IsLEDOn())	//check if LED is off before continuing
				m_data->mode = state::turn_off_led;
			else
				m_data->mode = nextState;
			std::cout << "so moving to " << stateName[m_data->mode] << std::endl;
			break;
		case state::calibration_done:
			last = Wait();
			m_data->mode = state::measurement;		//calibration done and saved, start measurement
			break;
		case state::measurement:
			last = Wait();
			if (IsMeasurementDone())			//check if calibration is completed
				nextState = state::measurement_done;	//if so, it can be saved to disk
			else
				nextState = state::measurement;	//if not, repeat calibration loop

			if (IsLEDOn())	//check if LED is off before continuing
				m_data->mode = state::turn_off_led;
			else
				m_data->mode = nextState;
			break;
		case state::measurement_done:
			last = Wait();
			m_data->mode = state::finalise;			//measurement done, turn on pump and change water for next measurement and finalise
			break;
		case state::turn_off_led:
			std::cout << "is led off?" << std::endl;
			if (IsLEDOn())	//check if LED is off before continuing
				m_data->mode = state::turn_off_led;
			else
				m_data->mode = nextState;
			std::cout << "so moving to " << stateName[m_data->mode] << std::endl;
			break;
		case state::finalise:
			last = Wait(change_water_time);			//wait for pump to complete if needed
			m_data->mode = state::power_down;		//turn off PSU
			break;
		case state::power_down:
			last = Wait(power_down_time);			//small wait to let PSU be down
			rest_time = idle_time;
			m_data->mode = state::idle;			//and move to idle
			break;
	}

	std::cout << "mode end = " << stateName[m_data->mode] << std::endl;

	return true;
}

bool Scheduler::Finalise()
{
	return true;
}

bool Scheduler::IsCalibrated()
{
	return m_data->isCalibrated;
}

bool Scheduler::IsCalibrationDone()
{
	return m_data->calibrationDone;
}

bool Scheduler::IsLEDOn()
{
	return m_data->isLEDon;
}

bool Scheduler::IsMeasurementDone()
{
	return m_data->measurementDone;
}

//check if enough time has passed since last status change
//wait remaining time so that in total t seconds have passed since last
//with t = 0, no wait happens and current time is passed
boost::posix_time::ptime Scheduler::Wait(double t)
{
	std::cout << "waiting " << t << std::endl;

	boost::posix_time::ptime current(boost::posix_time::second_clock::local_time());
	boost::posix_time::time_duration period(0, 0, t, 0);
	boost::posix_time::time_duration lapse(period - (current - last));

	if (!lapse.is_negative())			//if positive, wait!
		usleep(lapse.total_microseconds());

	std::cout << "done" << std::endl;

	return boost::posix_time::second_clock::local_time();
}
