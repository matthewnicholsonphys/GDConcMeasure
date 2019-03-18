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

GdTree::GdTree(const std::string &treeName)
{
	if (treeName.empty())
		chain = Create("_tree");
	else
		chain = Create(treeName);
	chain->SetDirectory(0);
}

GdTree::GdTree(TTree *tree)
{
	if (tree)
	{
		chain = tree;
		chain->SetDirectory(0);
		Init();
	}
	else
		GdTree("_tree");
}

GdTree::GdTree(const GdTree& gdtree)	//copy tree with same name, but empty
{
	Create(gdtree.chain->GetName());
}

GdTree::~GdTree()
{
	delete chain;
}

TTree* GdTree::GetTree()
{
	return chain;
}

void GdTree::Write(std::string outName)
{
	if (outName.empty())
		outName = chain->GetName();

	chain->Write(outName.c_str(), TObject::kWriteDelete);
}

void GdTree::Fill()
{
	chain->Fill();
}

int GdTree::GetEntry(long entry)
{
	if (!chain)
		return 0;

	return chain->GetEntry(entry);
}

int GdTree::GetEntries()
{
	if (!chain)
		return 0;

	return chain->GetEntries();
}

TTree* GdTree::Create(const std::string &treeName)
{
	TTree* chain = new TTree(treeName.c_str(), treeName.c_str());

	p_Wavelength	= &Wavelength;
	p_Trace		= &Trace;
	p_T_Err		= &T_Err;
	p_Absorbance	= &Absorbance;
	p_Absorb_Err	= &Absorb_Err;

	b_GdConc	= chain->Branch("GdConc",	&GdConc,	"GdConc/D");
	b_Gd_Err	= chain->Branch("Gd_Err",	&Gd_Err,	"Gd_Err/D");
	b_Gd_ErrStat	= chain->Branch("Gd_ErrStat",	&Gd_ErrStat,	"Gd_ErrStat/D");
	b_Gd_ErrSyst	= chain->Branch("Gd_ErrSyst",	&Gd_ErrSyst,	"Gd_ErrSyst/D");
	b_Wavelength_1	= chain->Branch("Wavelength_1",	&Wavelength_1,	"Wavelength_1/D");
	b_Absorbance_1	= chain->Branch("Absorbance_1",	&Absorbance_1,	"Absorbance_1/D");
	b_Absorb_Err_1	= chain->Branch("Absorb_Err_1",	&Absorb_Err_1,	"Absorb_Err_1/D");
	b_Wavelength_2	= chain->Branch("Wavelength_2",	&Wavelength_2,	"Wavelength_2/D");
	b_Absorbance_2	= chain->Branch("Absorbance_2",	&Absorbance_2,	"Absorbance_2/D");
	b_Absorb_Err_2	= chain->Branch("Absorb_Err_2",	&Absorb_Err_2,	"Absorb_Err_2/D");
	b_Absorb_Diff	= chain->Branch("Absorb_Diff",	&Absorb_Diff,	"Absorb_Diff/D");
	b_AbsDiff_Err	= chain->Branch("AbsDiff_Err",	&AbsDiff_Err,	"AbsDiff_Err/D");
	b_Trace		= chain->Branch("Wavelength",	&p_Wavelength);	//,"Wavelength/D");
	b_Trace		= chain->Branch("Trace",	&p_Trace);	//,"Trace/D");
	b_T_Err 	= chain->Branch("T_Err",	&p_T_Err);	//,"T_Err/D");	
	b_Absorbance 	= chain->Branch("Absorbance",	&p_Absorbance);	//,"Absorbance/D");
	b_Absorb_Err 	= chain->Branch("Absorb_Err",	&p_Absorb_Err);	//,"Absorb_Err/D");
	b_Epoch 	= chain->Branch("Epoch",	&Epoch,		"Epoch/l"  );
	b_Year		= chain->Branch("Year", 	&Year,		"Year/I");
	b_Month		= chain->Branch("Month",	&Month,		"Month/I");
	b_Day		= chain->Branch("Day",		&Day,		"Day/I");
	b_Hour		= chain->Branch("Hour",		&Hour,		"Hour/I");
	b_Minute	= chain->Branch("Minute",	&Minute,	"Minute/I");
	b_Second	= chain->Branch("Second",	&Second,	"Second/I");
	
	return chain;
}

void GdTree::Init()
{
	p_Wavelength	= &Wavelength;
	p_Trace		= &Trace;
	p_T_Err		= &T_Err;
	p_Absorbance	= &Absorbance;
	p_Absorb_Err	= &Absorb_Err;

	chain->SetBranchAddress("GdConc",	&GdConc,	&b_GdConc);
	chain->SetBranchAddress("Gd_Err",	&Gd_Err,	&b_Gd_Err);
	chain->SetBranchAddress("Gd_ErrStat",	&Gd_ErrStat,	&b_Gd_ErrStat);
	chain->SetBranchAddress("Gd_ErrSyst",	&Gd_ErrSyst,	&b_Gd_ErrSyst);
	chain->SetBranchAddress("Wavelength_1",	&Wavelength_1,	&b_Wavelength_1);
	chain->SetBranchAddress("Absorbance_1",	&Absorbance_1,	&b_Absorbance_1);
	chain->SetBranchAddress("Absorb_Err_1", &Absorb_Err_1,	&b_Absorb_Err_1);
	chain->SetBranchAddress("Wavelength_2", &Wavelength_2,	&b_Wavelength_2);
	chain->SetBranchAddress("Absorbance_2", &Absorbance_2,	&b_Absorbance_2);
	chain->SetBranchAddress("Absorb_Err_2", &Absorb_Err_2,	&b_Absorb_Err_2);
	chain->SetBranchAddress("Absorb_Diff",	&Absorb_Diff,	&b_Absorb_Diff);
	chain->SetBranchAddress("AbsDiff_Err",	&AbsDiff_Err,	&b_AbsDiff_Err);
	chain->SetBranchAddress("Wavelength",	&p_Wavelength,	&b_Wavelength);
	chain->SetBranchAddress("Trace",	&p_Trace,	&b_Trace);
	chain->SetBranchAddress("T_Err",	&p_T_Err,	&b_T_Err);
	chain->SetBranchAddress("Absorbance",	&p_Absorbance,	&b_Absorbance);
	chain->SetBranchAddress("Absorb_Err",	&p_Absorb_Err,	&b_Absorb_Err);
	chain->SetBranchAddress("Epoch",	&Epoch,		&b_Epoch);
	chain->SetBranchAddress("Year", 	&Year,		&b_Year);
	chain->SetBranchAddress("Month",	&Month,		&b_Month);
	chain->SetBranchAddress("Day",		&Day,		&b_Day);
	chain->SetBranchAddress("Hour",		&Hour,		&b_Hour);
	chain->SetBranchAddress("Minute",	&Minute,	&b_Minute);
	chain->SetBranchAddress("Second",	&Second,	&b_Second);
}
