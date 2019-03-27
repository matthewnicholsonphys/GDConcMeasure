//////////////////////////////////////////////////////////
// This class has been automatically generated on
// Thu Jan 24 16:07:15 2019 by ROOT version 6.14/06
// from TTree GdConc/GdConc
// found on file: Memory Directory
//////////////////////////////////////////////////////////

#ifndef GdConc_h
#define GdConc_h

#include <TROOT.h>
#include <TChain.h>
#include <TFile.h>

// Header file for the classes stored in the TTree if any.

class GdConc {

	public :
		TTree *fChain;   //!pointer to the analyzed TTree or TChain
		Int_t fCurrent;	//!current Tree number in a TChain

		// Fixed size dimensions of array or collections stored in the TTree if any.

		// Declaration of leaf types
		double Gd_Conc;
		double Gd_Conc_Err;
		double Wavelength_1;
		double Absorbance_1;
		double Absorb_Err_1;
		double Wavelength_2;
		double Absorbance_2;
		double Absorb_Err_2;
		double AbsorbDiff;
		double AbsDiff_Err;
		int    nTrace;
		double Trace[nTrace];   //[n]
		double Trace_Err[nTrace];   //[n]

		// List of branches
		TBranch *b_Gd_Conc;
		TBranch *b_Gd_Conc_Err;
		TBranch *b_Wavelength_1;
		TBranch *b_Absorbance_1;
		TBranch *b_Absorb_Err_1;
		TBranch *b_Wavelength_2;
		TBranch *b_Absorbance_2;
		TBranch *b_Absorb_Err_2;
		TBranch *b_AbsorbDiff;
		TBranch *b_AbsDiff_Err;
		TBranch *b_nTrace;
		TBranch *b_Trace;   //[n]
		TBranch *b_Trace_Err;   //[n]

		GdConc(TTree *tree=0);
		virtual ~GdConc();
		virtual Int_t    Cut(Long64_t entry);
		virtual Int_t    GetEntry(Long64_t entry);
		virtual Long64_t LoadTree(Long64_t entry);
		virtual void     Init(TTree *tree);
		virtual void     Loop();
		virtual Bool_t   Notify();
		virtual void     Show(Long64_t entry = -1);
};

#endif

#ifdef GdConc_cxx
GdConc::GdConc(TTree *tree) : fChain(0) 
{
	// if parameter tree is not specified (or zero), connect the file
	// used to generate this class and read the Tree.
	if (tree == 0)
	{
		TFile *f = (TFile*)gROOT->GetListOfFiles()->FindObject("Memory Directory");
		if (!f || !f->IsOpen()) {
			f = new TFile("Memory Directory");
		}
		TDirectory * dir = (TDirectory*)f->Get("Rint:/");
		dir->GetObject("GdConc",tree);
	}
	Init(tree);
}

GdConc::~GdConc()
{
	if (!fChain) return;
	delete fChain->GetCurrentFile();
}

Int_t GdConc::GetEntry(Long64_t entry)
{
	// Read contents of entry.
	if (!fChain) return 0;
	return fChain->GetEntry(entry);
}
Long64_t GdConc::LoadTree(Long64_t entry)
{
	// Set the environment to read one entry
	if (!fChain) return -5;
	Long64_t centry = fChain->LoadTree(entry);
	if (centry < 0) return centry;
	if (fChain->GetTreeNumber() != fCurrent) {
		fCurrent = fChain->GetTreeNumber();
		Notify();
	}
	return centry;
}

void GdConc::Init(TTree *tree)
{
	// The Init() function is called when the selector needs to initialize
	// a new tree or chain. Typically here the branch addresses and branch
	// pointers of the tree will be set.
	// It is normally not necessary to make changes to the generated
	// code, but the routine can be extended by the user if needed.
	// Init() will be called many times when running on PROOF
	// (once per file to be processed).

	// Set branch addresses and branch pointers
	if (!tree) return;
	fChain = tree;
	fCurrent = -1;
	fChain->SetMakeClass(1);

	fChain->SetBranchAddress("Gd_Conc",	 &Gd_Conc,	&b_Gd_Conc);
	fChain->SetBranchAddress("Gd_Conc_Err",	 &Gd_Conc_Err,	&b_Gd_Conc_Err);
	fChain->SetBranchAddress("Wavelength_1", &Wavelength_1, &b_Wavelength_1);
	fChain->SetBranchAddress("Absorbance_1", &Absorbance_1, &b_Absorbance_1);
	fChain->SetBranchAddress("Absorb_Err_1", &Absorb_Err_1, &b_Absorb_Err_1);
	fChain->SetBranchAddress("Wavelength_2", &Wavelength_2, &b_Wavelength_2);
	fChain->SetBranchAddress("Absorbance_2", &Absorbance_2, &b_Absorbance_2);
	fChain->SetBranchAddress("Absorb_Err_2", &Absorb_Err_2, &b_Absorb_Err_2);
	fChain->SetBranchAddress("AbsorbDiff",	 &AbsorbDiff,	&b_AbsorbDiff);
	fChain->SetBranchAddress("AbsDiff_Err",	 &AbsDiff_Err,	&b_AbsDiff_Err);
	fChain->SetBranchAddress("nTrace",	 &nTrace,	&b_nTrace);
	fChain->SetBranchAddress("Trace",	 Trace,		&b_Trace);
	fChain->SetBranchAddress("Trace_Err",	 Trace_Err,	&b_Trace_Err);

	Notify();
}

Bool_t GdConc::Notify()
{
	// The Notify() function is called when a new file is opened. This
	// can be either for a new TTree in a TChain or when when a new TTree
	// is started when using PROOF. It is normally not necessary to make changes
	// to the generated code, but the routine can be extended by the
	// user if needed. The return value is currently not used.

	return kTRUE;
}

void GdConc::Show(Long64_t entry)
{
	// Print contents of entry.
	// If entry is not specified, print current entry
	if (!fChain) return;
	fChain->Show(entry);
}
Int_t GdConc::Cut(Long64_t entry)
{
	// This function may be called from Loop.
	// returns  1 if entry is accepted.
	// returns -1 otherwise.
	return 1;
}
#endif // #ifdef GdConc_cxx
