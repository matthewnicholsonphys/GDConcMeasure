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

	return true;
}

bool Scheduler::Execute()
{
	//std::cout<<"mode="<<m_data->mode<<std::endl;
	switch (m_data->mode)
	{
		case state::idle:
			last = Wait(idle_time);				//ToolDAQ is put to sleep until next measurement
			m_data->mode = state::power_up;			//time to power up the PSU!	in power up, Power tool turns on PSU
			break;
		case state::power_up:					//PSU has been turned on
			last = Wait(power_up_time);			//small wait to let PSU be stable
			m_data->mode = state::init;			//init the other Tools (calibration, measurement, LED, spectrometer, analysis...)
			break;
		case state::init:					//ToolChain has been configured, and dark noise has been taken and saved
			last = Wait();
			if (IsCalibrated())				//Check if calibration is present (was done by CalibrationManager during previous loop)
				m_data->mode = state::measurement;	//if there is calibration, then schedluer skips to measurement
			else
				m_data->mode = state::calibration;	//if there isn't calibration, this one must be done first!
		case state::calibration:
			last = Wait();
			if (IsCalibrationDone())				//check if calibration is completed
				m_data->mode = state::calibration_done;	//if so, it can be saved to disk
			else
				m_data->mode = state::calibration;	//if not, repeat calibration loop
			break;
		case state::calibration_done:
			last = Wait();
			m_data->mode = state::measurement;		//calibration done and saved, start measurement
			break;
		case state::measurement:
			last = Wait();
			if (IsMeasurementDone())			//check if calibration is completed
				m_data->mode = state::measurement_done;	//if so, it can be saved to disk
			else
				m_data->mode = state::measurement;	//if not, repeat calibration loop
			Wait(idle_time);
			break;
		case state::measurement_done:
			last = Wait();
			m_data->mode = state::finalise;			//measurement done, turn on pump and change water for next measurement and finalise
			break;
		case state::change_water:
			last = Wait(change_water_time);			//wait for pump to complete if needed
			m_data->mode = state::power_down;		//turn off PSU
			break;
		case state::power_down:
			last = Wait(power_down_time);			//small wait to let PSU be down
			m_data->mode = state::idle;			//and move to idle
			break;
	}
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

bool Scheduler::IsMeasurementDone()
{
	return m_data->measurementDone;
}

//check if enought time has passed since last status change
//wait remaining time so that in total t seconds have passed since last
//with t = 0, no wait happens and current time is passed
boost::posix_time::ptime Scheduler::Wait(double t)
{
	boost::posix_time::ptime current(boost::posix_time::second_clock::local_time());
	boost::posix_time::time_duration period(0, 0, t, 0);
	boost::posix_time::time_duration lapse(period - (current - last));

	if (!lapse.is_negative())			//if positive, wait!
		usleep(lapse.total_microseconds());

	return boost::posix_time::second_clock::local_time();
}

/*
{
	if (m_data->mode == 1){
		//    std::cout<<"mode1"<<std::endl;
		boost::posix_time::ptime current(boost::posix_time::second_clock::local_time());
		boost::posix_time::time_duration duration(current - last);
		boost::posix_time::time_duration period(0,0,10,0);
		//    std::cout<<duration<<std::endl;
		if (duration<period) usleep(500000);
		else{
			m_data->mode=2;
			std::cout<<"Start Up"<<std::endl;
		}
	}

	else if (m_data->mode==2){
		// std::cout<<"mode2"<<std::endl;
		last=boost::posix_time::second_clock::local_time();
		m_data->mode=3; std::cout<<"Pumping"<<std::endl;
		//  std::cout<<"t"<<std::endl;
	}

	else if (m_data->mode==3){
		boost::posix_time::ptime current(boost::posix_time::second_clock::local_time());
		boost::posix_time::time_duration duration(current - last);
		boost::posix_time::time_duration period(0,0,5,0);
		//    std::cout<<duration<<std::endl;
		if (duration<period) usleep(500000);
		else{
			m_data->mode=4;
			last=boost::posix_time::second_clock::local_time();
			std::cout<<"Setteling"<<std::endl;
		}
	}

	else if (m_data->mode==4){
		boost::posix_time::ptime current(boost::posix_time::second_clock::local_time());
		boost::posix_time::time_duration duration(current - last);
		boost::posix_time::time_duration period(0,0,5,0);
		//    std::cout<<duration<<std::endl;
		if (duration<period) usleep(500000);
		else{
			m_data->mode=5;
			std::cout<<"Measuring"<<std::endl;
		}
	}    

	return true;
}
*/
