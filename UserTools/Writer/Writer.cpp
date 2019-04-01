#include "Writer.h"

Writer::Writer() : Tool()
{
}

bool Writer::Initialise(std::string configfile, DataModel &data)
{
	if(configfile!="")  m_variables.Initialise(configfile);
	//m_variables.Print();

	m_data= &data;

	return true;
}

bool Writer::Execute()
{
	switch (m_data->mode)
	{
		case state::init:
			darkTrace.clear();
			pureTrace.clear();
			vTraceCollect.clear();
			break;
			break;
		case state::calibration_done:		//outputFile & outputTree are calibration's
			WriteFunctions();
		case state::measurement:		//outputFile & outputTree are calibration's
		case state::measurement_done:		//outputFile & outputTree are measurement's
			WriteTree();
			break;
		default:
			break;
	}

	return true;
}

bool Writer::Finalise()
{
	return true;
}

void Witer::WriteTree()
{
	//some loop on GdTrees
	//and call GdTree->Write()
	//and at the same time erase GdTree from map
}

void Witer::WriteTree()
{
	TFile outf(m_data->GetGdTree(m_data->treeName)->Output().c_str(), "UPDATE");

	m_data->concentrationFunction->Write();
	m_data->concentrationFunc_Err->Write();
	outf.Close();
}
