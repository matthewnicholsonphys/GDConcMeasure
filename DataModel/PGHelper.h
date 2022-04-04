#ifndef PGHELPER_H
#define PGHELPER_H

#include "Postgres.h"
#include "Store.h"

class DataModel;

class PGHelper{
	public:
	PGHelper(DataModel* m_data_in=nullptr);
	~PGHelper();
	void SetDataModel(DataModel* m_data_in);
	void SetVerbosity(int verb);
	
	bool GetCurrentRun(int& runnum, int* runconfig=nullptr, std::string* err=nullptr);
	bool GetRunConfig(int& runconfig_in, int* runnum_in=nullptr, std::string* err=nullptr);
	bool GetToolsConfig(std::string& toolsconfig, int* runnum=nullptr, int* runconfig_in=nullptr, std::string* err=nullptr);
	bool GetToolConfig(std::string toolname, std::string& toolconfig, int* versionnum=nullptr, int* runconfig=nullptr, int* runnum=nullptr, std::string* err=nullptr);
	int InsertToolConfig(Store config, std::string toolname, std::string author, std::string description, std::string* err);
	
	private:
	DataModel* m_data;
	
	int verbosity=1;
	int v_error=0;
	int v_warning=1;
	int v_message=2;
	int v_debug=3;
	std::string logmessage;
	int get_ok;
	
};


#endif
