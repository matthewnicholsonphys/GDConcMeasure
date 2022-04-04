#include "GdTree.h"

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
