#include "BenSpectrometer.h"

BenSpectrometer::BenSpectrometer() : Tool(){}

bool BenSpectrometer::Initialise(std::string configfile, DataModel &data){
  
  if(configfile!=""){
    m_variables.Initialise(configfile);
  }
  //m_variables.Print();
  
  m_data = &data;
  
  m_variables.Get("traces", nTraces);
  m_variables.Get("integration", intTime);
  m_variables.Get("verbosity", verbosity);
  
  power="OFF";
  return true;
}


bool BenSpectrometer::Execute(){
  
  std::string Power="";
  
  if(m_data->CStore.Get("Power",Power) && Power!=power){
    
    if(Power=="ON"){
      power=Power;
      Log("spectrometer on",v_message,verbosity);
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
      
    }
    else if (Power=="OFF") Power=power;
  
  }
  
  std::string tmp="";
  
  if(m_data->CStore.Get("Measure",tmp) && tmp=="Start"){
    Log("got measure",v_message,verbosity);
    GetData();
  
  }
  
  
  return true;
}


bool BenSpectrometer::Finalise(){
  
  return true;
}


bool BenSpectrometer::EstablishUSB_try_catch(){
  
  error = 0;
  char nameBuffer[80];
  int flag=1;
  sbapi_initialize();
  sbapi_probe_devices();
  //sbapi_add_TCPIPv4_device_location("Blaze", "192.168.1.151", 57357);
  //sbapi_add_TCPIPv4_device_location("Jaz", "192.168.1.150", 7654);
  
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
  
  error = 0;
  char nameBuffer[80];
  int flag=1;
  sbapi_initialize();
  sbapi_probe_devices();
  sbapi_add_TCPIPv4_device_location("Blaze", "192.168.50.2", 57357);
  //sbapi_add_TCPIPv4_device_location("Jaz", "192.168.1.150", 7654);
  //sbapi_add_TCPIPv4_device_location("Jaz", "192.168.50.2", 7654);
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
  
  sbapi_close_device(device_ids[0], &error);
  free(device_ids);
  free(spectrometer_ids);
  sbapi_shutdown();
  
}

bool BenSpectrometer::GetData(){
  
  /*
   * take measurement with LED off
   * this should be background to following measurements
   * save in vTraceCollect all traces
   * save in xAxis the wavelength of x-axis
   */
  error = 0;
  double *data           = 0;
  double *wavelength = 0;
  
  int size = sbapi_spectrometer_get_formatted_spectrum_length(device_ids[0],spectrometer_ids[0], &error);
  Log("size="+std::to_string(size),v_message,verbosity);
  sbapi_spectrometer_set_integration_time_micros(device_ids[0], spectrometer_ids[0],&error, intTime);
  data           = (double *)calloc(size, sizeof(double));
  wavelength = (double *)calloc(size, sizeof(double));
  
  m_data->traceCollect.clear();
  for(int i = 0; i < nTraces; i++){
    
    sbapi_spectrometer_get_formatted_spectrum(device_ids[0], spectrometer_ids[0],&error, data, size);
    std::vector<double> trace(size);
    memcpy(&trace[0], &data[0], size*sizeof(double));
    m_data->traceCollect.push_back(trace);
  }
  
  sbapi_spectrometer_get_wavelengths(device_ids[0], spectrometer_ids[0],
                                     &error, wavelength, size);
  m_data->wavelength.resize(size);
  memcpy(&(m_data->wavelength)[0], &wavelength[0], size*sizeof(double));
  
  free(data);
  free(wavelength);
  
  return true;
}


