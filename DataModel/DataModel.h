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

#include "TFile.h"
#include "TF1.h"
#include "TF2.h"

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

		//TTree* GetTTree(std::string name);
		//void AddTTree(std::string name, TTree *tree);
		//void DeleteTTree(std::string name);

		GdTree* GetGdTree(std::string name);
		void AddGdTree(std::string name, GdTree *tree);
		void DeleteGdTree(std::string name);

		Store vars;
		BoostStore CStore;
		std::map<std::string,BoostStore*> Stores;

		Logging *Log;

		zmq::context_t* context;



		state mode;	//state for scheduler

		//Calibration manager
		//created and destroyed in the class
		//
		//	used globally
		std::string calibrationName;
		bool isCalibrated;	//true if there is calibratio file!
		bool calibrationDone;	//true if calibration is complete
		//
		//	used in Analysis
		std::string concentrationName;	//name of tree for concentration measurmenet
		TF1 *concentrationFunction;
		TF2 *concentrationFunc_Err;
		double gdconc, gd_err;	//value of gd concentration and error
		//
		//	used in Writing
		TFile* calibrationFile;

	
		//Calibration manager
		//created and destroyed in the class
		//
		//	used globally
		bool measurementDone;	//true if the measurement is finished
		//
		//	used in Writing
		TFile* measurementFile;
		//	used in analysis
		std::string measurementName;

		//LEDmanager
		//string used by calibration and measurement managers
		std::string turnOnLED;	//led to be turned on

		//Pump
		//
		//override scheduler
		bool turnOnPump;	//must change water


		//filled by Spectrometer, for Analysis
		//
		std::vector<std::vector<double> > traceCollect;
		std::vector<double> wavelength;	//can be retrieved from seabreeze

		std::map<std::string, GdTree*> m_gdtrees; 

	private:

		//std::map<std::string, TTree*> m_trees; 
};


#endif
