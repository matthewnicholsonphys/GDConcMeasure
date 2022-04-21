#include "BenLED.h"
#include "Algorithms.h"

BenLED::BenLED():Tool(){}


bool BenLED::Initialise(std::string configfile, DataModel &data){
  
  if(configfile!=""){
    m_configfile = configfile;
    m_variables.Initialise(configfile);
  }
  //m_variables.Print();
  
  m_data = &data;
  
  m_variables.Get("verbosity",verbosity);
  
  m_variables.Get("frequency", frequencyPWM);
  m_variables.Get("resolution", resolution);
  resolution = static_cast<unsigned int>(pow(2, resolution)) - 1;
  
  m_variables.Get("voltage_supply", fVin);
  m_variables.Get("delay", fDelay);
  m_variables.Get("map_wiring", wiringLED);
  
  Log("BenLED: LED wiring and setup will be read from " + wiringLED, v_message, verbosity);
  
  /*
  //check when mapping file was changed
  std::string cmd = "stat -c %Y " + wiringLED + " > .led_timestamp";
  int s = system(cmd.c_str());
  std::ifstream inTimestamp(".led_timestamp");
  unsigned long newTime = lastTime+1;        //initiliased randomly
  if (std::getline(inTimestamp, cmd))
    newTime = std::strtol(cmd.c_str(), NULL, 10);
  
  //if it hasn't been changed since last time, don't do anyhting
  if (newTime != lastTime)
  */
  
  // read LED pin mapping from config file
  MapLED();
  
  //lastTime = newTime;
  power="OFF";
  
  return true;
}

bool BenLED::Execute(){
  
  Log("BenLED Executing...", v_debug, verbosity);
  
  bool ok = true;
  std::string Power="";
  
  // check for a flag in the DataModel indicating we are powering up or down
  if(m_data->CStore.Get("Power",Power) && Power!=power){
    
    // udpate internal state variable so we can track changes
    power=Power;
    
    if(Power=="ON"){
      
      // on powerup, establish comms with the PWM board embedded in the detector end cap
      // this also sets the PWM modulation frequency
      ok = EstablishI2C();
      if(not ok) return ok;
      
      // note in CStore that we've established comms to the PWM board for indication on website
      m_data->CStore.Set("PWMBoardHandle",file_descript);
      
      // initialise all LEDs to off
      ok = TurnOffAll();
      if(not ok){
        Log("BenLED::Execute error Initializing LED states to off!",0,0);
        return false;
      }
    }
    
  }
  
  std::string LED;
  if(m_data->CStore.Get("LED",LED)){
    if(!file_descript){
      // not gonna get anywhere without a device handle
      Log("BenLED::Execute LED change called with null file_descriptor!",0,0);
      ok = false;
      
    } else if(LED=="Off All"){
      ok = TurnOffAll();
      if(ok){
        Log("all leds off",v_message,verbosity);
      } else {
        Log("BenLED::Execute error calling TurnOffAll!!! LEDs may not be off!",0,0);
      }
      
    } else if(LED=="Change"){
      
      //m_data->CStore.Print();
      std::string R_str="0";
      std::string G_str="0";
      std::string B_str="0";
      std::string White_str="0";
      std::string B385_str="0";
      std::string UV260_str="0";
      std::string UV275_str="0";
      m_data->CStore.Get("R",R_str);
      m_data->CStore.Get("G",G_str);
      m_data->CStore.Get("B",B_str);
      m_data->CStore.Get("White",White_str);
      m_data->CStore.Get("385",B385_str);
      m_data->CStore.Get("260",UV260_str);
      m_data->CStore.Get("275",UV275_str);
      
      // not the most robust way, but probably the best we can do.
      short R=(R_str=="1");
      short G=(G_str=="1");
      short B=(B_str=="1");
      short White=(White_str=="1");
      short B385=(B385_str=="1");
      short UV260=(UV260_str=="1");
      short UV275=(UV275_str=="1");
      
      if( R || G || B ){
        ok &= TurnLEDon("LEDRGBAnnode");
        ok &= TurnLEDon("LEDR");
        ok &= TurnLEDon("LEDG");
        ok &= TurnLEDon("LEDB");
      }
      else{
        ok &= TurnLEDoff("LEDRGBAnnode");
        ok &= TurnLEDoff("LEDR");
        ok &= TurnLEDoff("LEDG");
        ok &= TurnLEDoff("LEDB");
      }
      
      if(R)  ok &= TurnLEDoff("LEDR");
      if(G)  ok &= TurnLEDoff("LEDG");
      if(B)  ok &= TurnLEDoff("LEDB");
      
      
      if(White) ok &= TurnLEDon("LEDW");
      else ok &= TurnLEDoff("LEDW");
      if(B385) ok &= TurnLEDon("LED385L");
      else ok &= TurnLEDoff("LED385L");
      if(UV260) ok &= TurnLEDon("LED260J");
      else ok &= TurnLEDoff("LED260J");
      if(UV275) ok &= TurnLEDon("LED275J");
      else ok &= TurnLEDoff("LED275J");
      //delete entries after maybe??
    }
    m_data->CStore.Remove("LED");
   
  }
  
  
  return ok;
}

bool BenLED::TurnOnAll(){
  
  if (IsSleeping() && !WakeUpDriver()){
    Log("BenLED::TurnOnAll unable to wake driver!",0,0);
    return false;
  }
  
  bool ok = TurnLEDon("LEDRGBAnnode");
  
  // try to turn on all other LEDs
  for (std::map<std::string, unsigned int>::iterator it =  mLED_name.begin(); it != mLED_name.end(); ++it){
    ok &= TurnLEDon(it->first);
  }
  
  return ok;
}

bool BenLED::TurnOffAll(){
  // turn all LEDs off
  bool all_ok = true;
  
  // lazy &&, check if sleeping and wake up driver if so
  // return false if unable to wake driver.
  if (IsSleeping() && !WakeUpDriver()){
    Log("BenLED::TurnOffAll unable to wake driver!",0,0);
    return false;
  }
  
  // loop over LEDS and turn them off
  for (std::map<std::string, unsigned int>::iterator it =  mLED_name.begin(); it != mLED_name.end(); ++it){
    // special case; skip RGB LED - we don't turn the cathodes off, just the collective anode.
    if(it->first != "LEDRGBAnnode") all_ok &= TurnLEDoff(it->first);
  }
  // turn off RGB LED
  all_ok &= TurnLEDoff("LEDRGBAnnode");
  
  return all_ok;
}


bool BenLED::Finalise(){
  
  // turn off all LEDs... but only try this if we have an open connection to the PWM board
  bool ok = true;
  if (file_descript){
    ok = TurnOffAndSleep();        // 0 will turn off all LEDs
  }
  return ok;
}


void BenLED::MapLED(){
  // build maps of LED name to pin, voltage, and bit within a bitmap.
  Log("Reading LED pin mapping",v_message,verbosity);
  
  mLED_name.clear();
  mLED_chan.clear();
  mLED_duty.clear();
  
  std::ifstream inConfig(wiringLED.c_str());
  std::string line;
  //this, in binary, will be a string of 1 and 0 where 1 is LED on and 0 is LED off
  while (std::getline(inConfig, line)){        //reading configfile
    
    //removes all comments (anything after '#')
    if (line.find_first_of('#') != std::string::npos)
      line.erase(line.find_first_of('#'));
    
    std::stringstream ssl(line);
    
    std::string key;
    int pin;
    double volt;
    if (ssl >> key >> pin >> volt){
      
      // make an entry for this LED in 3 maps, using the LED name as key:
      // 1) pin the LED is connected to
      mLED_chan[key] = pin;
      // 2) LED target voltage; this is divided by the PWM supply voltage to convert to PWM duty cycle
      // quick sanity check that the requested voltage is not greater than the supply voltage
      if (volt > fVin)        Log("BenLED: WARNING\tvoltage of " + key + " set higher than Vin", 1, verbosity);
      else mLED_duty[key] = volt / fVin;
      // 3) map this LED to the next free bit in a bitmap. Allows all LED states to be specified by one int
      mLED_name[key] = static_cast<unsigned int>(pow(2, mLED_name.size()-1));
    
    }
  }
}

//this routine will setup I2C connection with first address found
//in I2C mapping by i2cdetect program
//
bool BenLED::EstablishI2C(){
  Log("Establishing I2C connection",v_message,verbosity);
  
  // scan i2c for devices and write result into a temporary file
  std::string errmsg;
  int ok = SystemCall("i2cdetect -y 1 > .led_wiring", errmsg);
  if(ok!=0){
    Log("BenLED::EstablishI2C "+errmsg,0,0);
    return false;
  }
  
  std::ifstream inWiring(".led_wiring");
  if(!inWiring.is_open()){
    Log("BenLED::EstablishI2C failed to find file .led_wiring!",v_error,verbosity);
    return false;
  }
  std::string line;
  int addr;
  bool address_found=false;
  
  // scan lines describing i2c devices found
  // i2cdetect returns a table of possible addresses, spanning 0-128, or in hex [0-7][0-F].
  // In the returned table rows are the most significant nibble, columns the least significant nibble.
  // e.g. address 0x73 would be row 7 column 3.
  // Addresses where no device responded contain '--', while ones where a device responded contain
  // the corresponding address (i.e. row 7 column 3 will contain '73').
  // We assume that we only have one i2c device on bus 1 (from i2cdetect -y 1)
  // and scan through the table, dropping the first row and column,
  // and reading until we find an entry that isn't '--'.
  while (std::getline(inWiring, line)){
    
    // skip header line (does not contain ':' character)
    int pos = line.find_first_of(':');
    if(pos == std::string::npos) continue;
    
    // drop first column (up to and including ':')
    line.erase(0, pos+1);
    
    // parse the remaining fields
    std::stringstream ssl(line);
    while (ssl >> line){
      // skip '--' fields - no device responded
      if(line == "--") continue;
      
      // other entries represent addresses at which a device responded.
      // read the address in base 16 (hex), and assume this is our PWM board.
      addr = std::strtol(line.c_str(), NULL, 16);
      address_found = true;
      break;
    }
    
    // break once we found something
    if(address_found) break;
  }
  
  if(!address_found){
    Log("BenLED::EstablishI2C failed to find any devices with i2cdetect!",0,0);
    return false;
  }
  
  // if we found a device,try to establish comms
  Log("BenLED making I2C connection to address " + std::to_string(addr), v_message, verbosity);
  
  file_descript = wiringPiI2CSetup(addr);
  if(file_descript<0){
    std::string errorstr = std::strerror(errno);  // errno is a macro #defined by <errno.h>
    Log("BenLED::EstablishI2C error invoking wiringPiI2CSetup: '"+errorstr+"'",0,0);
    return false;
  }
  
  // set PWM modulation frequency
  return SetPWMfreq(frequencyPWM);
}

bool BenLED::SetPWMfreq(double freq){
  
  // coerce to within limits of the PWM board
  if (freq < 24.0){
    Log("BenLED::SetPWMfreq coercing PWM modulation frequency from "+std::to_string(freq)
        +" to 24Hz (minimum)",1,0);
    freq = 24.0;
  } else if (freq > 1526.0){
    Log("BenLED::SetPWMfreq coercing PWM modulation frequency from "+std::to_string(freq)
        +" to 1526Hz (maximum)",1,0);
    freq = 1526.0;
  }
  
  int prescale = std::round(25.0e6 / resolution / freq) - 1;
  //std::cout << "setting " << freq << " to " << std::hex << prescale << std::endl;
  
  // 0xfe is the address where the prescaler of the PWM frequency is stored
  return Write(0xfe, prescale);
}

bool BenLED::TurnOffAndSleep(){
  
  bool ok = true;
  if (!IsSleeping()){
    
    // FIXME ledONstate is never set; here it is being read unitialized
    //if (ledONstate != 0){
      
      // probably safe to call TurnLEDoff even if LEDs are off already anyway
      ok = TurnLEDArray(0);
      if(not ok){
        //return false;    // FIXME if this failed, should we return? should we update the internal state?
      }
      ledONstate = 0;
      
    //}
    
    ok = SleepDriver();
    if(not ok){
      Log("BenLED::TurnOffAndSleep failed to SleepDriver!",0,0);
    }
  }
  
  return ok;
}

bool BenLED::IsSleeping(){
  
  int mode1;
  bool ret = Read(0x00, mode1);
  if (ret) return mode1 & 0x10;        //true if sleep
  else        return false;
  
}

bool BenLED::WakeUpDriver(){
  
  // to wake up device from sleep, write MODE1 register
  // first, read MODE1 register
  // if sleep == 1 then set sleep to 0. Sleep bit is 0x10
  // wait 500us
  // write 1 to restart and other bits
  // Auto Increment == 1
  // if Driver was put to sleep without turning off outputs,
  // the RESTART bit (7bit of MODE1) must be cleared by writing 1
  
  //read MODE1 first
  int mode1;
  bool ret = Read(0x00, mode1);
  if (!ret)  return false;
  
  bool restart = mode1 & 0x80;        //check if restart must be cleared
  mode1 = mode1 & 0xf;                //xxxx
  
  //clear SLEEP bit and enable AI        : 0010xxxx         <--- NO AUTO INCREMENT
  ret = Write(0x00, (0x0 << 4) | mode1);
  usleep(500);        //wait at lest 500 us to use driver
  
  //if restart is 1, write 1 to clear it
  //write 1 to AI bit to enable auto-increment        <--- NO AUTO INCREMENT
  if (ret && restart)  ret = Write(0x00, (0x80 << 4) | mode1);
  
  return ret;
}

//put driver in sleep mode by writing 1 to SLEEP bit
//in register MODE1
bool BenLED::SleepDriver(){
  
  int mode1;
  bool ret = Read(0x00, mode1);
  if (ret) return Write(0x00, mode1 | 0x10);
  else return false;
  
}

/* utility to turn on LEDs
 * ledOptions is an int, result of OR operation on LED enums
 * the OR concatenation will result, for instance, in 10010010 = 146
 * the routine will loop through the possible led and check
 * which ones are requested to be on (=1) or off (=0)
 */
bool BenLED::TurnLEDArray(unsigned int ledON){
  
  bool all_ok = true;
  for (iLname = mLED_name.begin(); iLname != mLED_name.end(); ++iLname){
    
    if (ledON & iLname->second) all_ok &= TurnLEDon(iLname->first);
    else all_ok &= TurnLEDoff(iLname->first);
  }
  
  usleep(200);                //stabilises LED
  return all_ok;
  
}

//set registers on PCA9685
bool BenLED::TurnLEDon(std::string ledName){
  
  if (IsSleeping() && !WakeUpDriver()){
    Log("BenLED::TurnLEDon failed to wake up driver!",0,0);
    return false;
  }
  
  Log("Turning on "+ledName,v_message,verbosity);
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
  //        which is delay
  int time2on  = 0xfff & static_cast<int>(resolution * fDelay);
  //whereas the LED_OFF stores the time from clock to OFF state
  //        which is delay+iduty
  int time2off = 0xfff & (time2on + iduty);
  
  //get least significant byte by truncation
  //get most significant byte by shifting right
  
  //std::vector<int> v_data;
  //v_data.push_back(time2on  & 0xff);             //LEDn_ON_L
  //v_data.push_back(time2on  >> 8);               //LEDn_ON_H
  //v_data.push_back(time2off & 0xff);             //LEDn_OFF_L
  //v_data.push_back(time2off >> 8);               //LEDn_OFF_H
  
  bool akg = Write(reg_LED, time2on  & 0xff);      //LEDn_ON_L
  akg &= Write(reg_LED + 1, time2on  >> 8);        //LEDn_ON_H
  akg &= Write(reg_LED + 2, time2off & 0xff);      //LEDn_OFF_L
  akg &= Write(reg_LED + 3, time2off >> 8);        //LEDn_OFF_H
  
  
  int data;
  Read(reg_LED, data);
  Read(reg_LED+1, data);
  Read(reg_LED+2, data);
  Read(reg_LED+3, data);
  
  // note current status of this LED in the CStore for display on the website
  std::map<std::string,bool> ledStatuses;
  m_data->CStore.Get("ledStatuses",ledStatuses);
  ledStatuses[ledName] = true;
  m_data->CStore.Set("ledStatuses",ledStatuses);
  
  if(!akg){
    Log("BenLED::TurnLEDon failed to turn on LED "+ledName,0,0);
  }
  return akg;
  //use wiringPI function to write to correct register
  //and at the same time check for errors
  //
  //return WriteAI(reg_LED, v_data);
}

bool BenLED::TurnLEDoff(std::string ledName){
  
  if (IsSleeping() && !WakeUpDriver()) return false;
  
  Log("Turning off "+ledName,v_message,verbosity);
  //use wiringPI to write to set LEDn_OFFi_H[4] = 1
  //which is the "always OFF" bit
  //can be achieved by writing 0x10 to register ??
  int reg_LED = 4 * mLED_chan[ledName] + 6;
  
  bool akg = Write(reg_LED, 0x00);        //LEDn_ON_L
  akg &= Write(reg_LED + 1, 0x00);        //LEDn_ON_H
  akg &= Write(reg_LED + 2, 0x00);        //LEDn_OFF_L
  akg &= Write(reg_LED + 3, 0x00);        //LEDn_OFF_H
  
  // note current status of this LED in the CStore for display on the website
  std::map<std::string,bool> ledStatuses;
  m_data->CStore.Get("ledStatuses",ledStatuses);
  ledStatuses[ledName]=false;
  m_data->CStore.Set("ledStatuses",ledStatuses);
  
  if(!akg){
    Log("BenLED::TurnLEDoff failed to turn off LED "+ledName,0,0);
  }
  return akg;
  //return Write(reg_LED, 0x00);
}


// I2C communication functions
//
//standard 8bit read
bool BenLED::Read(int reg, int &data){
  
  if (file_descript){
    
    data = wiringPiI2CReadReg8(file_descript, reg);
    if (data < 0) return false;
    else return true;
  
  } else {
    Log("BenLED::Read called with null i2c address!",0,0);
  }
  
  return false;
}

//standard 8bit write
bool BenLED::Write(int reg, int data){
  
  if (file_descript){
    
    int ret = wiringPiI2CWriteReg8(file_descript, reg, data & 0xff);
    if (ret < 0) return false;
    else return true;
  }
  
  else return false;
  
}

//uses the autoincrement option to read to sequential registers faster
bool BenLED::ReadAI(int reg, int num_reg, std::vector<int> &block){
  // unused function
  
  block.clear();
  if (file_descript){
    
    //read first register manually
    int data = wiringPiI2CReadReg8(file_descript, reg);
    if (data < 0) return false;
    
    block.push_back(data);
    
    //read following registers sequentially
    for (int i = 1; i < num_reg; ++i){
      
      data = wiringPiI2CRead(file_descript);
      if (data < 0)return false;
      block.push_back(data);
    }
    
    return true;
  }
  
  else return false;
  
}

//uses the autoincrement option to write to sequential registers faster
bool BenLED::WriteAI(int reg, const std::vector<int> &block){
  // unused function
  
  if (file_descript){
    
    //write first register manually
    int ret = wiringPiI2CWriteReg8(file_descript, reg, block.front() & 0xff);
    if (ret < 0) return false;
    int data = wiringPiI2CRead(file_descript);
    
    //write following registers sequentially
    for (int i = 1; i < block.size(); ++i){
      
      ret = wiringPiI2CWrite(file_descript, block.at(i) & 0xff);
      data = wiringPiI2CRead(file_descript);
      if (ret < 0)return false;
      
    }
    
    return true;
  }
  else return false;
  
}
