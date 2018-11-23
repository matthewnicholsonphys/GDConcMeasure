#include "../Unity.cpp"

Tool* Factory(std::string tool){
Tool* ret=0;

// if (tool=="Type") tool=new Type;
if (tool=="DummyTool") ret=new DummyTool;

if (tool=="Scheduler") ret=new Scheduler;
  if (tool=="LEDmanager") ret=new LEDmanager;
return ret;
}

