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

enum status
{
	idle		= 0,
	calibration	= 1,
	cal_conc_1	= -1,
	cal_conc_2	= -2,
	cal_conc_3	= -3,
	cal_conc_4	= -4,
	change_water	= 2,
	init_spectr	= 3,
	turnon,
	measure
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
