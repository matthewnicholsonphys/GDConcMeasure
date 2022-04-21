#include "BenSpectrometer.h"

BenSpectrometer::BenSpectrometer() : Tool(){}

bool BenSpectrometer::Initialise(std::string configfile, DataModel &data){
  
  if(configfile!=""){
    m_variables.Initialise(configfile);
  }
  //m_variables.Print();
  
  m_data = &data;
  
  // get Tool configuration
  m_variables.Get("traces", nTraces);
  m_variables.Get("integration", intTime);
  m_variables.Get("verbosity", verbosity);
  m_variables.Get("SpectrometerModel",SpectrometerModel);
  m_variables.Get("SpectrometerIP",SpectrometerIP);
  m_variables.Get("SpectrometerPort",SpectrometerPort);
  
  // default state power off
  power="OFF";
  return true;
}


bool BenSpectrometer::Execute(){
  
  Log("BenSpectrometer Executing...",v_debug,verbosity);
  
  // check if we've just changing power state
  std::string Power="";
  if(m_data->CStore.Get("Power",Power) && Power!=power){
    
    // update our internal variable so that we know to re-connect when its turned back on
    power=Power;
    
    if(Power=="ON"){
      // if we've just powered up, establish a connection
      Log("spectrometer on",v_message,verbosity);
      
      // make 50(!) attempts to connect
      int tries=0;
      while(!EstablishUSB()){
        tries++;
        Log("Attempt "+std::to_string(tries)+"/50",v_debug,verbosity);
        if(tries>=50){
          Log("couldnt establish connetion to spectrometer",v_error,verbosity);
          return false;
        }
        sleep(1);
      }
      
      // update connection status in DataModel for display on website
      m_data->CStore.Set("SpectrometerConnected",true);
    } else {
      // we'll need to reconnect on power-up
      m_data->CStore.Set("SpectrometerConnected",false);
    }
  
  }
  
  // check if we have a flag in the DataModel to start a measurement
  std::string tmp="";
  if(m_data->CStore.Get("Measure",tmp) && tmp=="Start"){
    
    Log("got measure",v_message,verbosity);
    // read measurement from spectrometer
    bool ok = GetData();
    if(not ok){
      // should we stop the measurement here, so that downstream Tools
      // do not see the 'Measure' flag and try to process non-existent data?
      m_data->CStore.Remove("Measure");
      return ok;
    }
  
  }
  
  
  return true;
}


bool BenSpectrometer::Finalise(){
  
  return true;
}


bool BenSpectrometer::EstablishUSB_try_catch(){
  // unused
  
  error = 0;
  char nameBuffer[80];
  int flag=1;
  sbapi_initialize();
  sbapi_probe_devices();
  //sbapi_add_TCPIPv4_device_location(SpectrometerModel.c_str(), SpectrometerIP.c_str(), SpectrometerPort);
  
  if (sbapi_get_number_of_device_ids()!=1) return false;
  device_ids = (long *)calloc(1, sizeof(long));
  sbapi_get_device_ids(device_ids, 1);
  flag *= sbapi_get_device_type(device_ids[0], &error, nameBuffer, 79);
  //flag *= sbapi_open_device(device_ids[0], &error);
  int trial = 0;
  while (trial < 4){
    try {
      Log("Connecting spectrometer, attempt "+std::to_string(trial),v_message,verbosity);
      sbapi_open_device(device_ids[0], &error);
      return true;
    }
    catch (const std::exception &e) {
      Log("Can't connect to spectrometer",v_error,verbosity);
      Log("Attempting, trial: "+std::to_string(++trial),v_error,verbosity);
    }
  }
  
  sbapi_open_device(device_ids[0], &error);
  
  if(flag == 0)  return false;
  
  spectrometer_ids = (long *)calloc(1, sizeof(long));
  sbapi_get_spectrometer_features(device_ids[0], &error, spectrometer_ids, 1); /// this may not be needed
  
  return true;
}

bool BenSpectrometer::EstablishUSB(){
  // attempt to open a connection to the spectrometer
  
  error = 0;
  char nameBuffer[80];
  int flag=1;
  sbapi_initialize();
  sbapi_probe_devices();
  sbapi_add_TCPIPv4_device_location(const_cast<char*>(SpectrometerModel.c_str()),
                                    const_cast<char*>(SpectrometerIP.c_str()),
                                    SpectrometerPort);
  if (sbapi_get_number_of_device_ids()!=1) return false;
  device_ids = (long *)calloc(1, sizeof(long));
  sbapi_get_device_ids(device_ids, 1);
  flag *= sbapi_get_device_type(device_ids[0], &error, nameBuffer, 79);
  Log("namebuffer="+std::string(nameBuffer),v_debug,verbosity);
  Log("error="+std::to_string(error),((error==0) ? v_debug : v_error),verbosity);
  //flag *= sbapi_open_device(device_ids[0], &error);
  sleep(30);
  sbapi_open_device(device_ids[0], &error);
  Log("error="+std::to_string(error),((error==0) ? v_debug : v_error),verbosity);
  if(flag ==0) return false;
  spectrometer_ids = (long *)calloc(1, sizeof(long));
  Log("spectrometer_ids="+std::to_string(*spectrometer_ids),v_debug,verbosity);
  Log("device_ids[0]="+std::to_string(device_ids[0]),v_debug,verbosity);
  sbapi_get_spectrometer_features(device_ids[0], &error, spectrometer_ids, 1); /// this may not be needed
  Log("error="+std::to_string(error),((error==0) ? v_debug : v_error),verbosity);
  Log("spectrometer_ids="+std::to_string(*spectrometer_ids),v_debug,verbosity);
  Log("device_ids[0]="+std::to_string(device_ids[0]),v_debug,verbosity);
  Log("established",v_debug,verbosity);
  return true;
}

void BenSpectrometer::RelinquishUSB(){
  // unused
  
  sbapi_close_device(device_ids[0], &error);
  free(device_ids);
  free(spectrometer_ids);
  sbapi_shutdown();
  
}

bool BenSpectrometer::GetData(){
  
  /*
   * take N spectrometer traces and stores result in DataModel
   */
  
  error = 0;
  double *data           = 0;
  double *wavelength = 0;
  
  // query amount of memory required to store a spectrum
  int size = sbapi_spectrometer_get_formatted_spectrum_length(device_ids[0],spectrometer_ids[0], &error);
  if(size==0 || error!=0){
    Log("BenSpectrometer::GetData failed getting trace length! sbapi_spectrometer_get_formatted_spectrum_length"
        " returned "+std::to_string(size)+", error code "+std::to_string(error),0,0);
    return false;
  }
  Log("size="+std::to_string(size),v_message,verbosity);
  
  // allocate storage
  data           = (double *)calloc(size, sizeof(double));
  wavelength = (double *)calloc(size, sizeof(double));
  
  // configure measurement integration time
  sbapi_spectrometer_set_integration_time_micros(device_ids[0], spectrometer_ids[0],&error, intTime);
  if(error){
    Log("BenSpectrometer::GetData sbapi_spectrometer_set_integration_time_micros returned error "
       +std::to_string(error),0,0);
    return 0;
  }
  
  // clear old data in DataModel
  m_data->traceCollect.clear();
  
  // loop over N traces (we'll average them later to reduce statistical uncertainty)
  for(int i = 0; i < nTraces; i++){
    
    // retrieve this spectrum
    int ret = sbapi_spectrometer_get_formatted_spectrum(device_ids[0], spectrometer_ids[0],&error, data, size);
    if(ret==0 || error!=0){
      Log("BenSpectrometer::GetData failed getting data! sbapi_spectrometer_get_formatted_spectrum returned "
          +std::to_string(ret)+", error code "+std::to_string(error),0,0);
      return false;
    }
    
    // convert to vector
    std::vector<double> trace(size);
    memcpy(&trace[0], &data[0], size*sizeof(double));
    
    // add this trace to datamodel vector
    m_data->traceCollect.push_back(trace);
  }
  
  // wavelengths are always the same so we only need one copy in the datamodel
  int ret = sbapi_spectrometer_get_wavelengths(device_ids[0], spectrometer_ids[0],
                                     &error, wavelength, size);
  if(ret==0 || error!=0){
    Log("BenSpectrometer::GetData failed getting wavevelengths! sbapi_spectrometer_get_wavelengths returned "
        +std::to_string(ret)+", error code "+std::to_string(error),0,0);
    return false;
  }
  m_data->wavelength.resize(size);
  memcpy(&(m_data->wavelength)[0], &wavelength[0], size*sizeof(double));
  
  // free temporaries for current trace
  free(data);
  free(wavelength);
  
  return true;
}


