#include "Spectrometer.h"

Spectrometer::Spectrometer() : Tool()
{
  //  startup=true;
}

bool Spectrometer::Initialise(std::string configfile, DataModel &data){
  
  //if(!startup){
    if(configfile!="")
      {
	m_configfile = configfile;
	m_variables.Initialise(configfile);
      }
    //m_variables.Print();
    
    m_data = &data;
    //}
  
// startup=false;
  return true;
}


bool Spectrometer::Execute()
{
	switch (m_data->mode){
	  
	case state::init:	//wake up, connect spectrometer on USB
	  //Initialise(m_configfile, *m_data);
	  if(!EstablishUSB()) return false;
	  GetData();	//measure dark noise on wake up
	  break;
	case state::calibration:
	  GetData();
	  break;
	case state::measurement:
	  GetData();
	  break;
	case state::finalise:
	  //Finalise();
	  RelinquishUSB();
	  break;
	default:
	  break;
	}
	
	return true;
}


bool Spectrometer::Finalise()
{
	return true;
}

bool Spectrometer::EstablishUSB()
{

  int error = 0;
  char nameBuffer[80];
  int flag=1;
  //std::cout<<"d1"<<std::endl;
  sbapi_initialize();
  //std::cout<<"d2"<<std::endl;
  sbapi_probe_devices();
  //std::cout<<"d3"<<std::endl;
  if (sbapi_get_number_of_device_ids()!=1) return false;
  //std::cout<<"d4"<<std::endl;
  device_ids = (long *)calloc(1, sizeof(long));
  //std::cout<<"d5"<<std::endl;
  // std::cout<<"get ids="<<
  sbapi_get_device_ids(device_ids, 1);
  //<<std::endl;
  //std::cout<<"d6"<<std::endl;
  flag *= sbapi_get_device_type(device_ids[0], &error, nameBuffer, 79);
  //std::cout<<"d7 "<<flag<<" , "<<sbapi_get_error_string(error)<<std::endl;
  // flag *= sbapi_open_device(device_ids[0], &error);
   sbapi_open_device(device_ids[0], &error);
  //std::cout<<"d8 "<<flag<<" , "<<sbapi_get_error_string(error)<<std::endl;
  if(flag ==0) return false;
  //std::cout<<"d9"<<std::endl;
  spectrometer_ids = (long *)calloc(1, sizeof(long));
  //std::cout<<"d10"<<std::endl;
  sbapi_get_spectrometer_features(device_ids[0], &error, spectrometer_ids, 1); /// this may not be needed
  
  return true;
}

bool Spectrometer::GetData()
{
	/*
	 * take measurement with LED off
	 * this should be background to following measurements
	 * save in vTraceCollect all traces
	 * save in xAxis the wavelength of x-axis
	 */
  //std::cout<<"in dark level"<<std::endl;
  int length;
  error = 0;
  // spectrometer_ids = 0;
  double *data = 0;
  //  std::vector<std::vector<double> > datavec;
  double *wavelength =0;

  //std::cout<<"b1"<<std::endl;
  length = sbapi_spectrometer_get_formatted_spectrum_length(device_ids[0],spectrometer_ids[0], &error);
  //sbapi_spectrometer_set_integration_time_micros(device_ids[0], spectrometer_ids[0], &error, time);
  //std::cout<<"b2"<<std::endl;
  data = (double *)calloc(length, sizeof(double));
  wavelength = (double *)calloc(length, sizeof(double));

  //std::cout<<"b3"<<std::endl;
  m_data->traceCollect.clear(); 
  for(int i=0;i<1000;i++){
    //    //std::cout<<"b4"<<std::endl;
    sbapi_spectrometer_get_formatted_spectrum(device_ids[0], spectrometer_ids[0], &error, data, length);
    // //std::cout<<"b5"<<std::endl;
    std::vector<double> tmp(length);
    // //std::cout<<"data[10]="<<data[10]<<std::endl;
  memcpy(&tmp[0], &data[0], length*sizeof(double));
  // datavec.push_back(tmp);
  m_data->traceCollect.push_back(tmp);
  // //std::cout<<"b6"<<std::endl;
  }
  //std::cout<<"b7"<<std::endl;
  sbapi_spectrometer_get_wavelengths(device_ids[0], spectrometer_ids[0], &error, wavelength, length);
  //std::cout<<"b8"<<std::endl;
  m_data->wavelength.resize(length);
  //std::cout<<"b9"<<std::endl;
  memcpy(&(m_data->wavelength)[0], &wavelength[0], length*sizeof(int));
  //std::cout<<"b10"<<std::endl;
  free(wavelength);
  //std::cout<<"b11"<<std::endl;
  free(data);
  //std::cout<<"b12"<<std::endl;

  
  return true;
  
}


void Spectrometer::RelinquishUSB(){

  sbapi_close_device(device_ids[0], &error);
  free(device_ids);
  free(spectrometer_ids);
  sbapi_shutdown();

}
