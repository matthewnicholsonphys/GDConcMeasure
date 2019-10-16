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
#include "TFitResult.h"
#include "TFitResultPtr.h"

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
	take_dark,
	take_spectrum,
	turn_off_led,
	analyse,
	change_water,
	settle_water,
	manual_on,
	manual_off,
	finalise
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
		int SizeGdTree();

		Store vars;
		BoostStore CStore;
		std::map<std::string,BoostStore*> Stores;

		Logging *Log;

		zmq::context_t* context;



		state mode;	//state for scheduler

		bool isCalibrationTool;
		bool isMeasurementTool;
		bool depleteWater;
		bool circulateWater;

		//Calibration manager
		//created and destroyed in the class
		//
		//	used globally
		std::string calibrationName;
		bool isCalibrated;	//true if there is calibratio file!
		bool calibrationDone;	//true if calibration is finished
		//bool calibrationComplete;	//true if calibration can be completed

		std::string calibrationTime;	//timestamp for calibration
		//
		//	used in Analysis
		std::string concentrationName;	//name of tree for concentration measurmenet
		//function accepts absorbance in and gives concentration
		TF1 *concentrationFunction;
		TF2 *concentrationFuncStat;	//y variable is error on absorbance
		TF1 *concentrationFuncSyst;
		TFitResultPtr concentrationFit;	//fit result
		std::string fitFuncName;	//name of fit result

		double gdconc, gd_err;	//value of gd concentration and error
		//
		//	used in Writing
		std::string calibrationFile;
		std::string calibrationBase;

	
		//Calibration manager
		//created and destroyed in the class
		//
		//	used globally
		bool measurementDone;	//true if the measurement is finished
		//
		//	used in Writing & used in analysis
		std::string measurementFile;
		std::string measurementName;
		std::string measurementBase;

		//LEDmanager
		//string used by calibration and measurement managers
		bool isLEDon;		//true if led on
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
