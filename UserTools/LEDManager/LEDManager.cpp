#include "LEDManager.h"

LEDManager::LEDManager():Tool(){}


bool LEDManager::Initialise(std::string configfile, DataModel &data)
{
	if(configfile!="")
	{
		m_configfile = configfile;
		m_variables.Initialise(configfile);
	}
	//m_variables.Print();
	
	m_data = &data;
	
	Configure();
	return EstablishI2C();	//it is true if connections is ok!
}

bool LEDManager::Execute()
{
	//int mode1;
	//bool test = Read(0x00, mode1);
	//std::cout << "register 1 is " << std::hex << mode1 << std::endl;

	//test = Read(0xfe, mode1);
	//std::cout << "frequencyscaler is " << std::hex << mode1 << std::endl;

	//WakeUpDriver();
	//test = Read(0x00, mode1);
	//std::cout << "awake and " << std::hex << mode1 << std::endl;

	//SleepDriver();
	//test = Read(0x00, mode1);
	//std::cout << "sleeping and " << std::hex << mode1 << std::endl;

	//for (iLname = mLED_name.begin(); iLname != mLED_name.end(); ++iLname)
	//{
	//	std::cout << iLname->first << "\t" << iLname->second << "\t" << int(mLED_chan[iLname->first]) << "\t" << mLED_duty[iLname->first] << std::endl;
	//}

	//measureCount = 0;
	//if (!measure && TurnOn())
	//	measure = true;
	

	switch (m_data->mode)
	{
		case state::init:	//about to make measurement, check LED mapping
			ledONstate = 0;
			Initialise(m_configfile, *m_data);
			break;
		case state::calibration:		//turn on led set
		case state::measurement:		//turn on led set
			TurnOn();
			break;
		case state::calibration_done:	//wait for measurement
		case state::measurement_done:	//wait for measurement
			TurnOff();
			break;
		case state::finalise:		//turn off in any other state
			TurnOffAndSleep();
			break;
	}

	return true;
}

bool LEDManager::Finalise()
{
	TurnOffAndSleep();	//0 will turn off all LEDs
	return true;
}

//configure the class reading from configfile
void LEDManager::Configure()
{
	lastTime = 0;
	ledONstate = 0;

	m_variables.Get("verbose", verbose);

	m_variables.Get("frequency", frequencyPWM);
	m_variables.Get("resolution", resolution);
	resolution = static_cast<unsigned int>(pow(2, resolution)) - 1;

	m_variables.Get("voltage_supply", fVin);
	m_variables.Get("delay", fDelay);
	m_variables.Get("map_wiring", wiringLED);
	m_variables.Get("measurement", configLED);

	Log("LEDManager: LED wiring and setup will be read from " + wiringLED, 1, verbose);
	Log("LEDManager: LED measurement will be read from " + configLED, 1, verbose);

	MapLED();
}

void LEDManager::MapLED()
{
	//check when mapping file was changed
	std::string cmd = "stat -c %Y " + wiringLED + " > .tmp_timestamp";
	int s = system(cmd.c_str());
	std::ifstream inTimestamp(".tmp_timestamp");
	unsigned long newTime = lastTime;
	if (std::getline(inTimestamp, cmd))
		newTime = std::strtol(cmd.c_str(), NULL, 10);

	//if it hasn't been changed since last time, don't do anyhting
	if (newTime == lastTime)
		return;

	//else update wire mapping
	lastTime = newTime;

	std::ifstream inConfig(wiringLED.c_str());
	std::string line;
	//this, in binary, will be a string of 1 and 0 where 1 is LED on and 0 is LED off 
	while (std::getline(inConfig, line))	//reading configfile
	{
		//removes all comments (anything after '#')
		if (line.find_first_of('#') != std::string::npos)
			line.erase(line.find_first_of('#'));

		std::stringstream ssl(line);

		std::string key;
		int pin;
		double volt;
		if (ssl >> key >> pin >> volt)
		{
			std::cout << key << "\t" << pin << "\t" << volt << std::endl;
			//name of LED is mapped to an incremental bit value
			mLED_name[key] = static_cast<unsigned int>(pow(2, mLED_name.size()-1));
			//name of LED is mapped to the pin it is connected to
			mLED_chan[key] = pin;
			//name of LED is mapped to its operative voltage
			//this must be later divided by input voltage to find duty cycle
			if (volt > fVin)
				Log("LEDManager: WARNING\tvoltage of " + key + " set higher than Vin", 1, verbose);
			else
				mLED_duty[key] = volt / fVin;
		}
	}
}

//this routine will setup I2C connection with first address found
//in I2C mapping by i2cdetect program
//
bool LEDManager::EstablishI2C()
{
	std::cout << "establishing connection" << std::endl;
	system("i2cdetect -y 1 > .tmp_wiring");
	std::ifstream inWiring(".tmp_wiring");
	std::string line;
	int addr;
	while (std::getline(inWiring, line))
	{
		int pos = line.find_first_of(':');
		if (pos != std::string::npos)
		{
			line.erase(0, pos+1);
			std::stringstream ssl(line);
			while (ssl >> line)
			{
				if (line != "--")
				{
					addr = std::strtol(line.c_str(), NULL, 16);
					goto address_found;
				}
			}
		}
	}

	address_found:
	Log("LEDManager: making I2C connection to address " + addr, 1, verbose);

	file_descript = wiringPiI2CSetup(addr);
	return SetPWMfreq(frequencyPWM);
}

bool LEDManager::SetPWMfreq(double freq)
{
	if (freq < 24.0)
		freq = 24.0;
	else if (freq > 1526.0)
		freq = 1526.0;

	int prescale = std::round(25.0e6 / resolution / freq) - 1;
	std::cout << "setting " << freq << " to " << std::hex << prescale << std::endl;

	//this address is is where the prescaler of the PWM frequency
	//is stored
	return Write(0xfe, prescale);
}

bool LEDManager::TurnOn()
{
	if (IsSleeping() && !WakeUpDriver())
		return false;

	unsigned int ledON = 0;

	std::stringstream ssl(m_data->turnOnLED);
	std::string led;
	while (std::getline(ssl, led, '_'))
		ledON = ledON | mLED_name[led];

	if (ledON != ledONstate)
	{
		ledONstate = ledON;
		return TurnLEDArray(ledONstate);	//if everything is ok, this returns true
	}
	else
		return true;		//continue measuring
}

bool LEDManager::TurnOff()
{
	if (!IsSleeping())
	{
		if (0 != ledONstate)
		{
			TurnLEDArray(0);
			ledONstate = 0;
		}

		return true;
	}
	else
		return true;
}
bool LEDManager::TurnOffAndSleep()
{
	if (!IsSleeping())
	{
		if (0 != ledONstate)
		{
			TurnLEDArray(0);
			ledONstate = 0;
		}

		return SleepDriver();
	}
	else
		return true;
}

bool LEDManager::IsSleeping()
{
	int mode1;
	bool ret = Read(0x00, mode1);
	if (ret)
		return mode1 & 0x10;	//true if sleep
	else
		return false;
}

bool LEDManager::WakeUpDriver()
{
	//write MODE1 register to wake up device from sleep
	//read frist MODE1 register
	//if sleep == 1 then set sleep to 0. Sleep bit is 0x10
	//wait 500us
	//write 1 to restart and other bits
	//Auto Increment == 1
	//if Driver was put to sleep without turning off outputs,
	//the RESTART bit (7bit of MODE1) must be cleared by writing 1
	
	//reade MODE1 first
	int mode1;
	bool ret = Read(0x00, mode1);
	if (!ret)
		return false;

	bool restart = mode1 & 0x80;	//check if restart must be cleared
	mode1 = mode1 & 0xf;	//xxxx

	//clear SLEEP bit and enable AI	: 0010xxxx 	<--- NO AUTO INCREMENT
	ret = Write(0x00, (0x0 << 4) | mode1);
	usleep(500);	//wait at lest 500 us to use driver

	//if restart is 1, write 1 to clear it
	//write 1 to AI bit to enable auto-increment	<--- NO AUTO INCREMENT
	if (ret && restart)
		ret = Write(0x00, (0x80 << 4) | mode1);

	return ret;
}

//put driver in sleep mode by writing 1 to SLEEP bit
//in register MODE1
bool LEDManager::SleepDriver()
{
	int mode1;
	bool ret = Read(0x00, mode1);
	if (ret)
		return Write(0x00, mode1 | 0x10);
	else
		return false;
}

/* utility to turn on LEDs
 * ledOptions is an int, result of OR operation on LED enums
 * the OR concatenation will result, for instance, in 10010010 = 146
 * the routine will loop through the possible led and check
 * which ones are requested to be on (=1) or off (=0) 
 */
bool LEDManager::TurnLEDArray(unsigned int ledON)
{
	for (iLname = mLED_name.begin(); iLname != mLED_name.end(); ++iLname)
	{
		//std::cout << std::hex << ledON << "\t" << iLname->second << std::endl;
		if (ledON & iLname->second)
			TurnLEDon(iLname->first);
		else
			TurnLEDoff(iLname->first);
	}
}

//set registers on PCA9685
bool LEDManager::TurnLEDon(std::string ledName)
{
	//std::cout << "turning on " << ledName << std::endl;
	//there are 4 registers to control LEDs
	//and they are [6+4*channel : 9+4*channel]
	//
	int reg_LED = 4 * mLED_chan[ledName] + 6;

	//use wiringPI to write to set LEDn_OFF_H[4] = 0
	//which is the "always OFF" bit
	//can be achieved by writing 0x00 to register ??
	//
	//Write(reg_OFF_H, 0x00);

	
	// the AND op with 0xFFF makes sure only 12-bit values are created
	int iduty = 0xfff & static_cast<int>(resolution * mLED_duty[ledName]);

	//the LED_ON register stores the time from clock to ON state
	//	which is delay
	int time2on  = 0xfff & static_cast<int>(resolution * fDelay);
	//whereas the LED_OFF stores the time from clock to OFF state
	//	which is delay+iduty
	int time2off = 0xfff & (time2on + iduty);

	//get least significant byte by truncation
	//get most significant byte by shifting right

	//std::vector<int> v_data;
	//v_data.push_back(time2on  & 0xff);	//LEDn_ON_L
	//v_data.push_back(time2on  >> 8);	//LEDn_ON_H
	//v_data.push_back(time2off & 0xff);	//LEDn_OFF_L
	//v_data.push_back(time2off >> 8);	//LEDn_OFF_H

	bool akg = Write(reg_LED, time2on  & 0xff);	//LEDn_ON_L
	akg &= Write(reg_LED + 1, time2on  >> 8);	//LEDn_ON_H
	akg &= Write(reg_LED + 2, time2off & 0xff);	//LEDn_OFF_L
	akg &= Write(reg_LED + 3, time2off >> 8);	//LEDn_OFF_H

	return akg;
	//use wiringPI function to write to correct register
	//and at the same time check for errors
	//
	//return WriteAI(reg_LED, v_data);
}

bool LEDManager::TurnLEDoff(std::string ledName)
{
	std::cout << "turning off " << ledName << std::endl;
	//use wiringPI to write to set LEDn_OFFi_H[4] = 1
	//which is the "always OFF" bit
	//can be achieved by writing 0x10 to register ??
	int reg_LED = 4 * mLED_chan[ledName] + 9;

	return Write(reg_LED, 0x00);
}




// I2C communication functions
//
//standard 8bit read
bool LEDManager::Read(int reg, int &data)
{
	if (file_descript)
	{
		data = wiringPiI2CReadReg8(file_descript, reg);
		if (data < 0)
			return false;
		else
			return true;
	}
	else
		return false;
}

//standard 8bit write
bool LEDManager::Write(int reg, int data)
{
	if (file_descript)
	{
		int ret = wiringPiI2CWriteReg8(file_descript, reg, data & 0xff);
		if (ret < 0)
			return false;
		else
			return true;
	}
	else
		return false;
}

//uses the autoincrement option to read to sequential registers faster
//
bool LEDManager::ReadAI(int reg, int num_reg, std::vector<int> &block)
{
	block.clear();
	if (file_descript)
	{
		//read first register manually
		int data = wiringPiI2CReadReg8(file_descript, reg);
		if (data < 0)
			return false;
		block.push_back(data);

		//read following registers sequentially
		for (int i = 1; i < num_reg; ++i)
		{
			data = wiringPiI2CRead(file_descript);
			if (data < 0)
				return false;
			block.push_back(data);
		}

		return true;
	}
	else
		return false;
}

//uses the autoincrement option to write to sequential registers faster
//
bool LEDManager::WriteAI(int reg, const std::vector<int> &block)
{
	if (file_descript)
	{
		//write first register manually
		int ret = wiringPiI2CWriteReg8(file_descript, reg, block.front() & 0xff);
		if (ret < 0)
			return false;
		int data = wiringPiI2CRead(file_descript);

		//write following registers sequentially
		for (int i = 1; i < block.size(); ++i)
		{
			ret = wiringPiI2CWrite(file_descript, block.at(i) & 0xff);
			data = wiringPiI2CRead(file_descript);
			if (ret < 0)
				return false;
		}

		return true;
	}
	else
		return false;
}
