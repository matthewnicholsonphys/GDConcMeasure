// This class has been automatically generated on
// Thu Jan 24 16:07:15 2019 by ROOT version 6.14/06
// from TTree GdTree/GdTree
// found on file: Memory Directory
//////////////////////////////////////////////////////////

#ifndef GdTree_h
#define GdTree_h

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
		double Wavelength_1;
		double Absorbance_1;
		double Absorb_Err_1;
		double Wavelength_2;
		double Absorbance_2;
		double Absorb_Err_2;
		double Absorb_Diff;
		double AbsDiff_Err;
		std::vector<double> Trace;
		std::vector<double> T_Err;
		std::vector<double> Absor;
		std::vector<double> A_Err;

		// List of branches
		TBranch *b_Gd_Conc;
		TBranch *b_Gd_Conc_Err;
		TBranch *b_Wavelength_1;
		TBranch *b_Absorbance_1;
		TBranch *b_Absorb_Err_1;
		TBranch *b_Wavelength_2;
		TBranch *b_Absorbance_2;
		TBranch *b_Absorb_Err_2;
		TBranch *b_Absorb_Diff;
		TBranch *b_AbsDiff_Err;
		TBranch *b_Trace;
		TBranch *b_T_Err;
		TBranch *b_Absor;
		TBranch *b_A_Err;

		GdTree(const std::string &treeName, const std::string &pathFile = 0);
		~GdTree();

		int GetEntry(Long64_t entry);
		int GetEntries();
		void Create(const std::string &treeName);
		void Init();
};

#endif

//save pathfile as output file to save tree
//constructor check if a tree exists already in the pathfile and uses that one
//if there is no tree
GdTree::GdTree(const std::string &treeName, const std::string &pathFile) :
	chain(0),
	outFile(pathFile)		//storing input/output file
{
	TFile infile(outFile.c_str(), "OPEN");
	chain = static_cast<TTree*>(infile.Get(treeName.c_str()));
	if (chain)
	{
		chain->SetDirectory(0);
		Init();
		infile.Close();
	}
	else
	{
		infile.Close();
		Create(treeName);
	}
}

GdTree::GdTree(const GdTree& gdtree) :
	chain(0),
	outFile(gdtree.outFile)
{
	Create(gdtree.chain->GetName());
}

GdTree::~GdTree()
{
	delete fChain;
}

std::string GdTree::Output()
{
	return outFile;
}

void GdTree::Write()
{
	TFile outfile(outFile.c_str(), "UPDATE");
	chain->Write("", TObject::kWriteDelete);
}

int GdTree::GetEntry(long entry)
{
	if (chain)
		return 0;
	return fChain->GetEntry(entry);
}

int GdTree::GetEntries()
{
	if (chain)
		return 0;
	return chain->GetEntries();
}

void GdTree::Create(const std::string &treeName)
{
	chain = new TTree(treeName.c_str(), treeName.c_str());

	b_Gd_Conc      = chain->Branch("GdConc",	&Gd_Conc);	//"GdConc/D");
	b_Gd_Conc_Err  = chain->Branch("Gd_Err",	&Gd_Conc_Err);	//"Gd_Err/D");
	b_Wavelength_1 = chain->Branch("Wavelength_1",	&Wavelength_1);	//"Wavelength_1gd/D");
	b_Absorbance_1 = chain->Branch("Absorbance_1",	&Absorbance_1);	//"Absorbance_1gd/D");
	b_Absorb_Err_1 = chain->Branch("Absorb_Err_1",	&Absorb_Err_1);	//"Absorb_Err_1gd/D");
	b_Wavelength_2 = chain->Branch("Wavelength_2",	&Wavelength_2);	//"Wavelength_2gd/D");
	b_Absorbance_2 = chain->Branch("Absorbance_2",	&Absorbance_2);	//"Absorbance_2gd/D");
	b_Absorb_Err_2 = chain->Branch("Absorb_Err_2",	&Absorb_Err_2);	//"Absorb_Err_2gd/D");
	b_Absorb_Diff  = chain->Branch("Absorb_Diff",	&Absorb_Diff);	//"Absorb_Diff/D");
	b_AbsDiff_Err  = chain->Branch("AbsDiff_Err",	&AbsDiff_Err);	//"AbsDiff_Err/D");
	b_Trace	       = chain->Branch("Trace",		&Trace);	//"Trace/D");
	b_T_Err	       = chain->Branch("T_Err",		&T_Err);	//"T_Err/D");	
	b_Absor	       = chain->Branch("Absor",		&Absor);	//"Absor/D");
	b_A_Err	       = chain->Branch("A_Err",		&A_Err);	//"A_Err/D");	
}

void GdTree::Init()
{
	// The Init() function is called when the selector needs to initialize
	// a new tree or chain. Typically here the branch addresses and branch
	// pointers of the tree will be set.
	// It is normally not necessary to make changes to the generated
	// code, but the routine can be extended by the user if needed.
	// Init() will be called many times when running on PROOF
	// (once per file to be processed).

	// Set branch addresses and branch pointers
	//if (!tree)
	//	return;

	//fChain = tree;
	//fCurrent = -1;
	//fChain->SetMakeClass(1);

	chain->SetBranchAddress("Gd_Conc",	&Gd_Conc,      &b_Gd_Conc);
	chain->SetBranchAddress("Gd_Conc_Err",	&Gd_Conc_Err,  &b_Gd_Conc_Err);
	chain->SetBranchAddress("Wavelength_1", &Wavelength_1, &b_Wavelength_1);
	chain->SetBranchAddress("Absorbance_1", &Absorbance_1, &b_Absorbance_1);
	chain->SetBranchAddress("Absorb_Err_1", &Absorb_Err_1, &b_Absorb_Err_1);
	chain->SetBranchAddress("Wavelength_2", &Wavelength_2, &b_Wavelength_2);
	chain->SetBranchAddress("Absorbance_2", &Absorbance_2, &b_Absorbance_2);
	chain->SetBranchAddress("Absorb_Err_2", &Absorb_Err_2, &b_Absorb_Err_2);
	chain->SetBranchAddress("Absorb_Diff",	&Absorb_Diff,  &b_Absorb_Diff);
	chain->SetBranchAddress("AbsDiff_Err",	&AbsDiff_Err,  &b_AbsDiff_Err);
	chain->SetBranchAddress("Trace",	&Trace,	       &b_Trace);
	chain->SetBranchAddress("A_Err",	&A_Err,	       &b_A_Err);
	chain->SetBranchAddress("Absor",	&Absor,	       &b_Absor);
	chain->SetBranchAddress("A_Err",	&A_Err,	       &b_A_Err);
}
