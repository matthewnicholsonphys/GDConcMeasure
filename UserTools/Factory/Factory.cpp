#include "../Unity.cpp"

Tool* Factory(std::string tool){
Tool* ret=0;

// if (tool=="Type") tool=new Type;
if (tool=="DummyTool") ret=new DummyTool;

if (tool=="Scheduler") ret=new Scheduler;
if (tool=="LEDManager") ret=new LEDManager;
if (tool=="Power") ret=new Power;
if (tool=="Spectrometer") ret=new Spectrometer;
if (tool=="Pump") ret=new Pump;
//if (tool=="Calibration") ret=new Calibration;
//if (tool=="Measurement") ret=new Measurement;
if (tool=="Analysis") ret=new Analysis;
if (tool=="Output") ret=new Output;
if (tool=="Writer") ret=new Writer;
if (tool=="CalibrationManager") ret=new CalibrationManager;
if (tool=="MeasurementManager") ret=new MeasurementManager;
return ret;
}

