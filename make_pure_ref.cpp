#include "TFile.h"
#include "TGraphErrors.h"
#include <iostream>

int main(int argc, const char** argv){
	if(argc<3){
		std::cerr<<"usage: "<<argv[0]<<" <file> <ledname> {<version>}"<<std::endl;
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
	std::string version  = (argc >3) ? argv[4] : "0";
	std::string psqlstring = "psql -c \"INSERT INTO data (timestamp, tool, ledname, name, values) VALUES ( 'now()', 'MarcusAnalysis', '"
	                        +ledname+"', 'pure_curve', '{\\\"version\\\":" + version + ", \\\"xvals\\\":[";
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
	psqlstring += "}' );\" ";
	std::cout<<std::endl<<psqlstring<<std::endl;
	return 0;
}
