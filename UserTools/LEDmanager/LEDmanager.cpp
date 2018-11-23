#include "LEDmanager.h"

LEDmanager::LEDmanager():Tool(){}


bool LEDmanager::Initialise(std::string configfile, DataModel &data)
{
	if(configfile!="")
		m_variables.Initialise(configfile);
	//m_variables.Print();
	
	m_data= &data;
	
	Configure();
	EstablishI2C();
	return true;
}

bool LEDmanager::Execute()
{

	switch (mode)
	{
		case ON:
			TurnOn();
			break;
		case OFF:
			TurnOff();
			break;
		default:
			break;
	}

	return true;
}

bool LEDmanager::Finalise()
{
	TurnOff();	//0 will turn off all LEDs
	return true;
}


//this routine will setup I2C connection with first address found
//in I2C mapping by i2cdetect program
//
void LEDmanager::EstablishI2C()
{
	system("i2cdetect -y1 > .tmp_wiring");
	std::ifstream inWiring (".tmp_wiring");
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
				if (line != "--")
				{
					addr = std::strtol(line, NULL, 16);
					goto address_found;
				}
		}

	}

	address_found:
		std::cout << "LEDmanager::EstablishI2C making connection to address " << std::hex << addr << std::endl;

	file_descript = wiringPiI2CSetup(addr);
	SetPWDfreq(frequencyPWM);
}

void LEDmanager::Configure(std::string configfile)
{
	std::ifstream inConfig(configfile.c_str());
	std::string line, key;
	double value;
	int pin;
	while (std::getline(inConfig, line))	//reading configfile
	{
		//removes all comments (anything after '#')
		if (line.find_first_of('#') != std::string::npos)
			line.erase(line.find_first_of('#'));

		std::stringstream ssl(line);
		
		if (ssl >> key >> value >> pin)
		{
			if (mLED_name.count(key))	//LED name already present!
				std::cerr << "LEDmanager::Configure WARNING: "
					  << ledName << " already exists in your config!" << std::endl;
			else
			{
				//name of LED is mapped to an incremental bit value
				mLED_name[key] = pow(2, mLEDs.size());
				//name of LED is mapped to its operative voltage
				//this must be later divided by input voltage to find duty cycle
				mLED_duty[key] = value;
				//name of LED is mapped to the pin it is connected to
				mLED_chan[key] = pin;
			}
		}
		else if (ssl >> key >> value)
		{
			//input voltage of external power supply
			if (key == "Voltage_supply")
				fVin = value;
			//delay (in duty cycle) before turning on
			else if (key == "Delay")
				fDelay = value;
			//counter resolution (in bit) usually 12 bit
			else if (key == "Resolution")
				iCounts = static_cast<unsigned int>(pow(2, value)) - 1;
			//PWM frequency
			else if (key == "Frequency")
				//uset wiringPI routines to set appropriate frequency
				frequencyPWM = value;
			else
				std::cerr << "LEDmanager::Configure WARNING: "
					  << key << " not recognised!" << std::endl;
		}
		else if (ssl >> key >> line)
		{
			if (key == "LEDon_config")
				configLEDon = line;
			std::cout << "LEDmanager::Configure " <<
				  << " LED to be used will be read from " << configLEDon << "\n"
				  << " Make sure the file is set in-between measurement" << std::endl;
		}
	}

	//divide voltages by Vin to find duty cycle %
	for (iLduty = mLED_duty.begin(); iLduty != mLED_duty.end(); ++iLduty)
	{
		if (iLduty->second > fVin)
		{
			std::cerr << "LEDmanager::Configure WARNING: "
				  << iLduty->first << " is set for a voltage bigger than " << fVin << " V\n"
				  << "                               it won't be used." << std::endl;
			iLduty->second = 0.0;
		}
		else
			iLduty->second /= fVin;
	}
}

void LEDmanager::TurnOn();
{
	WakeUpDriver();

	unsigned int ledON = WhichLEDon();
	if (ledON != ledONstatus)
	{
		TurnLEDArray(ledON);
		ledONstatus = ledON;
	}
}

void LEDmanager::TurnOff();
{
	if (0 != ledONstatus)
	{
		TurnLEDArray(0);
		ledONstatus = 0;
	}

	SleepDriver();
}

void LEDmanager::WakeUp()
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
	int mode1 = Read(0x00);
	if (mode1 & 0x80)
	{
		//clear SLEEP bit
		Write(mode1 | ~0x10);
		usleep(500);		//wait at lest 500 us to use driver

		//write 1 to RESTART bit to restart PWM
		//write 1 to AI bit to enable auto-increment
		Write(0xA0);	//turn sleep to 0
	}
}

//put driver in sleep mode by writing 1 to SLEEP bit
//in register MODE1
void LEDmanager::SleepDriver()
{
	int mode1 = Read(0x00);
	Write(mode1 | 0x10);
}

unsigned int LEDmanager::WhichLEDon()
{
	std::ifstream inLEDconfig(configLEDon.c_str());
	std::string line, key;

	unsigned int ledON = 0;
	//this, in binary, will be a string of 1 and 0 where 1 is LED on and 0 is LED off 
	while (std::getline(inLEDconfig, line))	//reading configfile
	{
		//removes all comments (anything after '#')
		if (line.find_first_of('#') != std::string::npos)
			line.erase(line.find_first_of('#'));

		std::stringstream ssl(line);

		while (ssl)
		{
			if (ssl >> key)
			{
				/* if there is key check if we know the key
				 * then apply OR operation with current ledON state
				 * at the end we are left with something like 011001
				 * which means turn on led 0 (LSB), 3, and 4 (MSB)
				 */
				iLname = mLED_name.find(key);
				if (iLname != mLED_name.end())
					ledON = ledON | iLname->second;
			}
		}
	}

	return ledON;
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

void LEDmanager::SetPWMfreq(double freq)
{
	int prescale = int(25e6 / iCounts / freq) - 1;

	//this address is is where the prescaler of the PWM frequency
	//is stored
	Write(0xfe, prescale);
}

//set registers on PCA9685
void LEDmanager::TurnLEDon(std::string ledName)
{
	if (changeLEDregisters)
	{
		//there are 4 registers to control LEDs
		//and they are [6+4*channel : 9+4*channel]
		//
		int reg_LED = 4 * mLED_chan[name] + 6;
		//int reg_ON_H  = reg_off + 1;
		//int reg_OFF_L = reg_off + 2;
		//int reg_OFF_H = reg_off + 3;

		//use wiringPI to write to set LEDn_OFF_H[4] = 0
		//which is the "always OFF" bit
		//can be achieved by writing 0x00 to register ??
		//
		//Write(reg_OFF_H, 0x00);

		
		// the AND op with 0xFFF makes sure only 12-bit values are created
		int iduty = 0xfff & static_cast<int>(iCounts * mLED_duty[ledName]);

		//the LED_ON register stores the time from clock to ON state
		//	which is delay
		int time2on  = 0xfff & static_cast<int>(iCounts * fDelay);
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
		WriteAI(reg_LED, v_data);
	}
}

void LEDmanager::TurnLEDoff(std::string ledName)
{
	if (changeLEDregisters)
	{
		//use wiringPI to write to set LEDn_OFFi_H[4] = 1
		//which is the "always OFF" bit
		//can be achieved by writing 0x10 to register ??
		int reg_LED = 4 * mLED_chan[name] + 9;
		Write(reg_LED, 0x00);
	}
}




// I2C communication functions
//

int LEDmanager::Read(int reg)
{
	if (file_decsript)
	{
		int data = wiringPiI2CReadReg8(file_descript, reg);
		if (data == -1)
			std::cerr << "LEDmanager::Read ERROR encountered " << errno << 
				  << "                 while reading reg " << std::hex << reg << std::endl;
		return data;
	}
	else
		return -1;
}

//standard 8bit write
int LEDmanager::Write(int reg, int data)
{
	if (file_decsript)
	{
		int ret = wiringPiI2CWriteReg8(file_descript, reg, data & 0xFF);
		if (ret == -1)
			std::cerr << "LEDmanager::Write ERROR encountered " << errno << 
				  << "                  while writing to reg " << std::hex << reg << std::endl;
		return ret;
	}
	else
		return -1;
}

//uses the autoincrement option to write to sequential registers faster
//
int LEDmanager::WriteAI(int reg, std::vector<int> &block_data)
{
	if (file_decsript)
	{
		int ret = Write(reg, block_data.front());
		for (int i = 1; i < block_data.size(); ++i)
			int ret = wiringPiI2CWrite(file_descript, block_data.front() & 0xFF);

		if (ret == -1)
			std::cerr << "LEDmanager::WriteAI ERROR encountered " << errno << 
				  << "                  while writing to reg " << std::hex << reg << std::endl;
		return ret;
	}
	else
		return -1;
}

//uses the autoincrement option to read to sequential registers faster
//
std::vector<int> LEDmanager::ReadAI(int reg, int num_reg)
{
	std::vector<int> block_data;
	if (file_decsript)
	{
		int data = Read(reg);
		block_data.push_back(data);

		for (int i = 1; i < num_reg; ++i)
		{
			data = wiringPiI2CRead(file_descript);
			block_data.push_back(data);
		}

		if (data == -1)
			std::cerr << "LEDmanager::WriteAI ERROR encountered " << errno << 
				  << "                  while writing to reg " << std::hex << reg << std::endl;
	}

	return block_data;
}
