#include "Scheduler.h"

Scheduler::Scheduler():Tool(){}

bool Scheduler::Initialise(std::string configfile, DataModel &data)
{
	if(configfile!="")
		m_variables.Initialise(configfile);
	//m_variables.Print();

	m_data = &data;

	m_data->mode = status::idle;

	return true;
}


bool Scheduler::Execute()
{
	//std::cout<<"mode="<<m_data->mode<<std::endl;
	switch (m_data->mode)
	{
		case status::idle:				//in idle nothing works
			last = Wait(idle_time);
			m_data->mode = status::power_up;	//time to power up the PSU
			break;
		case status::power_up:				//enables PowerTool that turns on PSU
			last = Wait(power_up_time);		//(small wait)
			m_data->mode = status::change_water;	//activate pump and interface with spectrometer
			break;
		case status::change_water:			//dark noise has been taken and water has been changed
			last = Wait(change_water_time);		//(depends on set up, cuvette, etc)
			if (isCalibrated())
				m_data->mode = status::measure;
			else
				m_data->mode = status::calibrate;
		case status::calibration:
			Wait(idle_time);
			break;
		case status::idle:
			Wait(idle_time);
			break;
	}
}
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


bool Scheduler::Finalise(){

	return true;
}

void Scheduler::Configure()
{
	m_variables.Get("verbose", verbose);

	m_variables.Get("idle",		idle_time);
	m_variables.Get("power_up",	power_up_time);
	m_variables.Get("change_water", water_time);
	m_variables.Get("calibration",	calibration_time);
	m_variables.Get("measurement",	measurement_time);
	m_variables.Get("turnon",	turnon_time);
	m_variables.Get("measure",	measure_time);
}

//wait t seconds using boost lib
//the DAQ can't do anything while waiting (?)
//check if enough time has passed since last time
boost::posix_time::second_clock Scheduler::Wait(double t)
{
	boost::posix_time::ptime current(boost::posix_time::second_clock::local_time());
	boost::posix_time::time_duration period(0, 0, t, 0);
	boost::posix_time::time_duration lapse(period - (current - last));

	if (!lapse.is_negative())			//if positive, wait!
		usleep(lapse.total_microseconds());

	return boost::posix_time::second_clock::local_time();
}
