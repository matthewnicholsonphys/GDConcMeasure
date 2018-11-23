#ifndef LEDmanager_H
#define LEDmanager_H

#include <string>
#include <iostream>
#include <errno.h>

#include "Tool.h"

enum LED
{
	led_260 = 1,
	led_275 = 2,
	led_385 = 4,
	led_405 = 8,
	led_W7  = 16,
	led_R   = 32,
	led_G   = 64,
	led_B   = 128,
};

class LEDmanager: public Tool
{
	public:
	
		LEDmanager();
		bool Initialise(std::string configfile,DataModel &data);
		bool Execute();
		bool Finalise();
	
	
	private:
		//map between led name and its "binary" value
		std::map<std::string, unsigned int> mLED_name;
		std::map<std::string, unsigned int>::iterator iLname;

		//map between led name and its gpIO pin
		std::map<std::string, unsigned char> mLED_chan;
		std::map<std::string, unsigned char>::iterator iLchan;

		//map between led name and its gpIO pin
		std::map<std::string, double> mLED_duty;
		std::map<std::string, double>::iterator iLduty;
		
		int file_descript;
		bool changeLEDregisters;
		int ledONstatus;
		double frequencyPWM;
};

#endif
