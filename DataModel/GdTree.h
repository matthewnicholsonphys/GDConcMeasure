// This class has been automatically generated on
// Thu Jan 24 16:07:15 2019 by ROOT version 6.14/06
// from TTree GdTree/GdTree
// found on file: Memory Directory
//////////////////////////////////////////////////////////

#ifndef GdTree_h
#define GdTree_h

#include <string>

#include <TROOT.h>
#include <TChain.h>
#include <TFile.h>

// Header file for the classes stored in the TTree if any.

class GdTree {
	public :
		TTree *chain;	//!pointer to the analyzed TTree or TChain

		// Declaration of leaf types
		double GdConc;
		double Gd_Err;
		double Gd_ErrStat;
		double Gd_ErrSyst;
		double Wavelength_1;
		double Absorbance_1;
		double Absorb_Err_1;
		double Wavelength_2;
		double Absorbance_2;
		double Absorb_Err_2;
		double Absorb_Diff;
		double AbsDiff_Err;
		std::vector<double> Wavelength;
		std::vector<double> Trace;
		std::vector<double> T_Err;
		std::vector<double> Absorbance;
		std::vector<double> Absorb_Err;
		std::vector<double> *p_Wavelength;
		std::vector<double> *p_Trace;
		std::vector<double> *p_T_Err;
		std::vector<double> *p_Absorbance;
		std::vector<double> *p_Absorb_Err;
		ULong64_t Epoch;
		int Year;
		int Month;
		int Day;
		int Hour;
		int Minute;
		int Second;

		// List of branches
		TBranch *b_GdConc;
		TBranch *b_Gd_Err;
		TBranch *b_Gd_ErrStat;
		TBranch *b_Gd_ErrSyst;
		TBranch *b_Wavelength_1;
		TBranch *b_Absorbance_1;
		TBranch *b_Absorb_Err_1;
		TBranch *b_Wavelength_2;
		TBranch *b_Absorbance_2;
		TBranch *b_Absorb_Err_2;
		TBranch *b_Absorb_Diff;
		TBranch *b_AbsDiff_Err;
		TBranch *b_Wavelength;
		TBranch *b_Trace;
		TBranch *b_T_Err;
		TBranch *b_Absorbance;
		TBranch *b_Absorb_Err;
		TBranch *b_Epoch;
		TBranch *b_Year;
		TBranch *b_Month;
		TBranch *b_Day;
		TBranch *b_Hour;
		TBranch *b_Minute;
		TBranch *b_Second;

		GdTree(const std::string &treeName);
		GdTree(TTree *tree = 0);
		GdTree(const GdTree& gdtree);
		~GdTree();

		TTree* GetTree();
		void Write(std::string outName = "");
		void Fill();
		int GetEntry(long entry);
		int GetEntries();
		TTree* Create(const std::string &treeName);
		void Init();
};

#endif
