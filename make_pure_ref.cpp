#include "TFile.h"
#include "TGraphErrors.h"
#include <iostream>

int main(int argc, const char** argv){
	if(argc<3){
		std::cerr<<"usage: "<<argv[0]<<" <file> <ledname> <date>"<<std::endl;
		return 0;
	}
	TFile* f = TFile::Open(argv[1]);
	if(f==nullptr){
		std::cerr<<"couldn't open file "<<argv[1]<<std::endl;
		return 1;
	}
	TGraph* g = (TGraph*)f->Get("Graph");
	if(g==nullptr){
		std::cerr<<"no graph named 'Graph' in file "<<argv[1]<<std::endl;
		return 2;
	}
	
	std::string ledname = argv[2];
	std::string date  = (argc >3) ? argv[3] : "now()";
	
	std::cout<<"# This script only generates the psql command to run; it does not make the database entry.\n"
	         <<"# Please validate the below printout and run the command if everything looks good\n"<<std::endl;
	
	std::string psqlstring = "psql -c \"INSERT INTO data (timestamp, tool, ledname, name, values) VALUES "
	                         "( '"+date+"', 'MarcusAnalysis', '" +ledname + "', 'pure_curve', '{ \\\"xvals\\\":[";
	for(int i=0; i<g->GetN(); ++i){
		psqlstring += std::to_string(g->GetX()[i]) + ",";
	}
	psqlstring.pop_back(); // remove trailing ','
	psqlstring += "], \\\"yvals\\\":[";
	for(int i=0; i<g->GetN(); ++i){
		psqlstring += std::to_string(g->GetY()[i]) + ",";
	}
	psqlstring.pop_back(); // remove trailing ','
	psqlstring += "]";
	
	TGraphErrors* ge = dynamic_cast<TGraphErrors*>(g);
	if(ge!=nullptr){
		// add errors if we have them
		psqlstring += ", \\\"xerrs\\\":[";
		for(int i=0; i<g->GetN(); ++i){
			psqlstring += std::to_string(g->GetEX()[i]) + ",";
		}
		psqlstring.pop_back(); // remove trailing ','
		psqlstring += "], \\\"yerrs\\\":[";
		for(int i=0; i<g->GetN(); ++i){
			psqlstring += std::to_string(g->GetEY()[i]) + ",";
		}
		psqlstring.pop_back(); // remove trailing ','
		psqlstring += "]";
	}
	psqlstring += ", \\\"version\\\":";   //+ version
	
	// inline next version number by querying for it
	std::string cmd = "psql -At -c \"SELECT MAX((values->>'version')::integer)+1 FROM data WHERE name='pure_curve' "
	                  "AND ledname='" + ledname + "'\" | tr -d '\n' ";
	std::cout<<psqlstring<<std::flush;
	system(cmd.c_str());
	std::cout<<"}' );\" "<<std::endl;
	
	return 0;
}
