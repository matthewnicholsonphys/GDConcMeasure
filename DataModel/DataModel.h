#ifndef DATAMODEL_H
#define DATAMODEL_H

#include <map>
#include <string>
#include <vector>

#include "TTree.h"

#include "Store.h"
#include "BoostStore.h"
#include "Logging.h"

#include <zmq.hpp>

enum state
{
	idle,
	init,
	calibrate,
	calibrate_dark,
	calibrate_pure,
	calibrate_gd,
	calibrate_fit,
	measure,
	measure_start,
	measure_stop
};

class DataModel {


 public:
  
  DataModel();

  TTree* GetTTree(std::string name);
  void AddTTree(std::string name,TTree *tree);
  void DeleteTTree(std::string name);

  Store vars;
  BoostStore CStore;
  std::map<std::string,BoostStore*> Stores;
  
  Logging *Log;

  zmq::context_t* context;

  //status mode;	//state for scheduler
  int mode;	//state for scheduler
  bool endMeasure;

  
  // pointer to spectromiter
  
 private:

  
  std::map<std::string,TTree*> m_trees; 
  
  
  
};



#endif
