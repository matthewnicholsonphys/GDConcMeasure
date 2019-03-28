#ifndef DATAMODEL_H
#define DATAMODEL_H

#include <map>
#include <string>
#include <vector>

#include "TTree.h"

#include "Store.h"
#include "BoostStore.h"
#include "Logging.h"
#include "GdTree.h"

#include <zmq.hpp>

enum state
{
	idle,
	power_up,
	power_down,
	init,
	calibration,
	calibration_done,
	measurement,
	measurement_done,
	finalise,
	change_water = finalise
};

class DataModel
{
	public:

		DataModel();

		TTree* GetTTree(std::string name);
		void AddTTree(std::string name, TTree *tree);
		void DeleteTTree(std::string name);

		GdTree* GetGdTree(std::string name);
		void AddGdTree(std::string name, GdTree *tree);
		void DeleteGdTree(std::string name);

		Store vars;
		BoostStore CStore;
		std::map<std::string,BoostStore*> Stores;

		Logging *Log;

		zmq::context_t* context;



		state mode;	//state for scheduler

		//for spectrometer
		std::vector<std::vector<double> > vTraceCollect;
		std::vector<double> xAxis;

		TF1 *concentrationFunction;
		TF2 *concentrationFunc_Err;

		double gdconc, gd_err;	//value of gd concentration and error

		bool isCalibrated;	//true if there is calibratio file!
		bool calibrationDone;	//true if calibration is complete
		bool measurementDone;	//true if the measurement is finished

		std::map<std::string, unsigned int> LED_name;	//led configurations
		unsigned int ledON;


	private:

		std::map<std::string, TTree*> m_trees; 
		std::map<std::string, GdTree*> m_gdtrees; 


};


#endif
