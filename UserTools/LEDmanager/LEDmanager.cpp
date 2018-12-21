#include "LEDmanager.h"

LEDmanager::LEDmanager():Tool(){}


bool LEDmanager::Initialise(std::string configfile, DataModel &data)
{
	if(configfile!="")
		m_variables.Initialise(configfile);
	//m_variables.Print();
	
	m_data= &data;
	
	Configure();
	return EstablishI2C();	//it is true if connections is ok!
}

bool LEDmanager::Execute()
{
	int mode1;
	bool test = Read(0x00, mode1);
	std::cout << "register 1 is " << mode1 << std::endl;

	test = Read(0xfe, mode1);
	std::cout << "frequencyscaler is " << mode1 << std::endl;

	WakeUpDriver();
	test = Read(0x00, mode1);
	std::cout << "awake and " << mode1 << std::endl;

	SleepDriver();
	test = Read(0x00, mode1);
	std::cout << "sleeping and " << mode1 << std::endl;

	/*
	switch (m_data->mode)
	{
		case status::init_spectr:	//about to make measurement, check LED mapping
			ledONstatus = 0;
			measureCount = 0;
			MapLED();
			break;
		case status::turnon:		//turn on led set
			if (!TurnOn())		//if TurnOn is false, measurement is finished or there is a uncaught problem
				m_data->endMeasure = true;
			break;
		case status::measure:	//wait for measurement
			break;
		default:		//turn off in any other state
			TurnOff();
			break;
	}
	*/


	return true;
}

bool LEDmanager::Finalise()
{
	TurnOff();	//0 will turn off all LEDs
	return true;
}

//configure the class reading from configfile
void LEDmanager::Configure()
{
	lastTime = 0;
	awake = false;
	ledONstatus = 0;

	m_variables.Get("verbose", verbose);

	m_variables.Get("frequency", frequencyPWM);
	m_variables.Get("resolution", iResolution);
	iResolution = static_cast<unsigned int>(pow(2, iResolution)) - 1;

	m_variables.Get("voltage_supply", fVin);
	m_variables.Get("delay", fDelay);
	m_variables.Get("map_wiring", wiringLED);
	m_variables.Get("measurement", configLED);

	Log("LEDManager: LED wiring and setup will be read from " + wiringLED, 1, verbose);
	Log("LEDManager: LED measurement will be read from " + configLED, 1, verbose);

	MapLED();
}

void LEDmanager::MapLED()
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
			//name of LED is mapped to an incremental bit value
			mLED_name[key] = pow(2, mLED_name.size());
			//name of LED is mapped to the pin it is connected to
			mLED_chan[key] = pin;
			//name of LED is mapped to its operative voltage
			//this must be later divided by input voltage to find duty cycle
			if (volt > fVin)
				Log("LEDmanager: WARNING\tvoltage of " + key + " set higher than Vin", 1, verbose);
			else
				mLED_duty[key] = volt / fVin;
		}
	}
}

//this routine will setup I2C connection with first address found
//in I2C mapping by i2cdetect program
//
bool LEDmanager::EstablishI2C()
{
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
				std::cout << line << std::endl;
				if (line != "--")
				{
					addr = std::strtol(line.c_str(), NULL, 16);
					goto address_found;
				}
			}
		}
	}

	address_found:
	std::cout << "wentto " << addr << std::endl;
	Log("LEDmanager: making I2C connection to address " + addr, 1, verbose);

	std::cout << "setting up" << std::endl;
	file_descript = wiringPiI2CSetup(addr);
	std::cout << "done" << std::endl;
	return SetPWMfreq(frequencyPWM);
}

bool LEDmanager::SetPWMfreq(double freq)
{
	int prescale = int(25e6 / iResolution / freq) - 1;

	//this address is is where the prescaler of the PWM frequency
	//is stored
	return Write(0xfe, prescale);
}

bool LEDmanager::TurnOn()
{
	if (!awake)
	{
		WakeUpDriver();
		awake = true;
	}

	std::ifstream inLED(configLED.c_str());
	std::string line;
	int i = 0;
	while (i < measureCount+1 && std::getline(inLED, line))	//skip to line measureCount+1
		++i;

	int ledON = 0;
	if (i == measureCount+1)
	{
		std::stringstream ssl(line);		//setup binary value
		while(ssl)
		{
			ssl >> line;
			ledON += mLED_name[line];
		}

		++measureCount;
	}
	else
		return false;		//stop measuring

	if (ledON != ledONstatus)
	{
		ledONstatus = ledON;
		return TurnLEDArray(ledONstatus);	//if everything is ok, this returns true
	}
	else
		return true;		//continue measuring
}

bool LEDmanager::TurnOff()
{
	if (awake)
	{
		if (0 != ledONstatus)
		{
			TurnLEDArray(0);
			ledONstatus = 0;
		}

		awake = false;
		return SleepDriver();
	}
	else
		return true;
}

bool LEDmanager::WakeUpDriver()
{
	//write MODE1 register to wake up device from sleep
	//read frist MODE1 register
	//if sleep == 1 then set sleep to 0. Sleep bit is 0x10
	//wait 500us
	//write 1 to restart and other bits
	//Auto Increment == 1
	//1010 0000
	//
	//read RESTART bit (x000 0000) of MODE1 register
	//it must be one
	int mode1;
	bool ret = Read(0x00, mode1);
	if (ret && (mode1 & 0x80))
	{
		//clear SLEEP bit
		ret = Write(0x00, mode1 | ~0x10);
		if (!ret)
			return false;

		usleep(500);		//wait at lest 500 us to use driver

		//write 1 to RESTART bit to restart PWM
		//write 1 to AI bit to enable auto-increment
		ret = Write(0x00, 0xA0);	//turn sleep to 0
	}

	return ret;
}

//put driver in sleep mode by writing 1 to SLEEP bit
//in register MODE1
bool LEDmanager::SleepDriver()
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
 * e.g.		      2		     16		  128
 * ledOptions = mLEDs["275"] | mLEDs["W7"]| mLEDs["B"]
 * the OR concatenation will result, for instance, in 10010010 = 146
 * the routine will loop through the possible led and check
 * which ones are requested to be on (=1) or off (=0) 
 */
bool LEDmanager::TurnLEDArray(unsigned int ledON)
{
	for (iLname = mLED_name.begin(); iLname != mLED_name.end(); ++iLname)
	{
		if (ledON & iLname->second)
			TurnLEDon(iLname->first);
		else
			TurnLEDoff(iLname->first);
	}
}

//set registers on PCA9685
bool LEDmanager::TurnLEDon(std::string ledName)
{
	//there are 4 registers to control LEDs
	//and they are [6+4*channel : 9+4*channel]
	//
	int reg_LED = 4 * mLED_chan[ledName] + 6;
	//int reg_ON_H  = reg_off + 1;
	//int reg_OFF_L = reg_off + 2;
	//int reg_OFF_H = reg_off + 3;

	//use wiringPI to write to set LEDn_OFF_H[4] = 0
	//which is the "always OFF" bit
	//can be achieved by writing 0x00 to register ??
	//
	//Write(reg_OFF_H, 0x00);

	
	// the AND op with 0xFFF makes sure only 12-bit values are created
	int iduty = 0xfff & static_cast<int>(iResolution * mLED_duty[ledName]);

	//the LED_ON register stores the time from clock to ON state
	//	which is delay
	int time2on  = 0xfff & static_cast<int>(iResolution * fDelay);
	//whereas the LED_OFF stores the time from clock to OFF state
	//	which is delay+iduty
	int time2off = 0xfff & (time2on + iduty);

	//get least significant byte by truncation
	//char LEDn_ON_L = static_cast<char>(time2on);
	////get most significant byte by shifting right
	//char LEDn_ON_H = static_cast<char>(time2on >> 8);

	////get least significant byte by truncation
	//char LEDn_OFF_L = static_cast<char>(time2off);
	////get most significant byte by shifting right
	//char LEDn_OFF_H = static_cast<char>(time2off >> 8);

	std::vector<int> v_data;
	v_data.push_back(time2on  & 0xff);
	v_data.push_back(time2on  >> 8);
	v_data.push_back(time2off & 0xff);
	v_data.push_back(time2off >> 8);

	//use wiringPI function to write to correct register
	//and at the same time check for errors
	//
	return WriteAI(reg_LED, v_data);
}

bool LEDmanager::TurnLEDoff(std::string ledName)
{
	//use wiringPI to write to set LEDn_OFFi_H[4] = 1
	//which is the "always OFF" bit
	//can be achieved by writing 0x10 to register ??
	int reg_LED = 4 * mLED_chan[ledName] + 9;

	return Write(reg_LED, 0x00);
}




// I2C communication functions
//
//standard 8bit read
bool LEDmanager::Read(int reg, int &data)
{
	if (file_descript)
	{
		data = wiringPiI2CReadReg8(file_descript, reg);
		if (data < 0)
		{
			Log("LEDmanager::Read ERROR " + errno, 0, verbose);
			return false;
		}
		else
			return true;
	}
	else
		return false;
}

//standard 8bit write
bool LEDmanager::Write(int reg, int data)
{
	if (file_descript)
	{
		int ret = wiringPiI2CWriteReg8(file_descript, reg, data & 0xFF);
		if (ret < 0)
		{
			Log("LEDmanager::Write ERROR " + errno, 0, verbose);
			return false;
		}
		else
			return true;
	}
	else
		return false;
}

//uses the autoincrement option to read to sequential registers faster
//
bool LEDmanager::ReadAI(int reg, int num_reg, std::vector<int> &block)
{
	block.clear();
	if (file_descript)
	{
		//read first register manually
		int data = wiringPiI2CReadReg8(file_descript, reg);
		if (data < 0)
		{
			Log("LEDmanager::ReadAI ERROR " + errno, 0, verbose);
			return false;
		}
		block.push_back(data);

		//read following registers sequentially
		for (int i = 1; i < num_reg; ++i)
		{
			data = wiringPiI2CRead(file_descript);
			if (data < 0)
			{
				Log("LEDmanager::ReadAI ERROR " + errno, 1, verbose);
				return false;
			}
			block.push_back(data);
		}

		return true;
	}
	else
		return false;
}

//uses the autoincrement option to write to sequential registers faster
//
bool LEDmanager::WriteAI(int reg, const std::vector<int> &block)
{
	if (file_descript)
	{
		//write first register manually
		int ret = wiringPiI2CWriteReg8(file_descript, reg, block.front() & 0xFF);
		if (ret < 0)
		{
			Log("LEDmanager::WriteAI ERROR " + errno, 0, verbose);
			return false;
		}

		//write following registers sequentially
		for (int i = 1; i < block.size(); ++i)
		{
			ret = wiringPiI2CWrite(file_descript, block.at(i) & 0xFF);
			if (ret < 0)
			{
				Log("LEDmanager::WriteAI ERROR " + errno, 0, verbose);
				return false;
			}
		}

		return true;
	}
	else
		return false;
}
