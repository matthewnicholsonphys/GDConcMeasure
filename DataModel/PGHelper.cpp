#include "PGHelper.h"
#include "DataModel.h"
#include <sstream>
#include <exception>

PGHelper::PGHelper(DataModel* m_data_in) : m_data(m_data_in) {};

void PGHelper::SetDataModel(DataModel* m_data_in){
	m_data = m_data_in;
}

void PGHelper::SetVerbosity(int verb){
	verbosity=verb;
}

bool PGHelper::GetCurrentRun(int& runnum, int* runconfig, std::string* err){
	if(verbosity>v_debug) std::cout<<"Getting current run"<<std::endl;
	
	// query the latest run number from the run database
	std::string query_string = "SELECT max(runnum) FROM run;";
	if(verbosity>v_debug) std::cout<<"Querying max run number"<<std::endl;
	get_ok = m_data->postgres.ExecuteQuery(query_string, runnum);
	if(not get_ok){
		std::string errmsg="failed to get max runnum from run table";
		std::cerr<<errmsg<<std::endl;
		if(err) *err = errmsg;
		return false;
	}
	if(verbosity>v_debug) std::cout<<"Max run number is "<<runnum<<std::endl;
	
	// if we're given somewhere to put the runconfig, get that as well while we're at it.
	if(runconfig){
		query_string = "SELECT runconfig FROM run WHERE runnum = "+pqxx::to_string(runnum);
		if(verbosity>v_debug) std::cout<<"Getting runconfig with query: \n"<<query_string<<"\n";
		get_ok = m_data->postgres.ExecuteQuery(query_string, *runconfig);
		if(not get_ok){
			std::string errmsg="failed to get runconfig for run "+std::to_string(runnum);
			std::cerr<<errmsg<<std::endl;
			if(err) *err = errmsg;
			return false;
		}
		if(verbosity>v_debug) std::cout<<"run config ID: "<<runconfig<<std::endl;
	}
	
	// return success
	return true;
}

bool PGHelper::GetRunConfig(int& runconfig, int* runnum_in, std::string* err){
	// Get the run config ID for a given run number. If no run number is given,
	// the run config ID in m_data->vars will be used.
	if(verbosity>v_debug) std::cout<<"getting runconfig for run "<<runnum_in<<std::endl;
	
	// first we need a run number. see if we're given one
	if(runnum_in==nullptr || *runnum_in<0){
		// not given one.
		if(verbosity>v_debug) std::cout<<"no run number; getting runconfig from vars"<<std::endl;
		
		/*
		// Assume the latest run in the rundb
		if(verbosity>v_debug) std::cout<<"No run number given, looking up latest run"<<std::endl;
		get_ok = GetCurrentRun(*runnum_in, &runconfig, err);
		// this gets the runconfig at the same time, and will populate any error, so we're done.
		*/
		
		// take it from m_data->vars
		get_ok = m_data->vars.Get("RunConfig",runconfig);
		if(not get_ok){
			std::string errmsg = "GetRunConfig called with no run number, but failed to Get "
			                     "RunConfig from ToolChainConfig";
			std::cerr<<errmsg<<std::endl;
			if(err) *err=errmsg;
		}
		
		return get_ok;
	}
	
	// else given a run number to use; query db for corresponding runconfig.
	std::string query_string = "SELECT runconfig FROM run WHERE runnum = "+ pqxx::to_string(*runnum_in)+";";
	get_ok = m_data->postgres.ExecuteQuery(query_string, runconfig);
	if(not get_ok){
		std::string errmsg="failed to get runconfig for run "+std::to_string(*runnum_in);
		std::cerr<<errmsg<<std::endl;
		if(err) *err = errmsg;
		return false;
	}
	if(verbosity>v_debug) std::cout<<"run config ID: "<<runconfig<<std::endl;
	
	return true;
}

bool PGHelper::GetToolsConfig(std::string& toolsconfig, int* runnum, int* runconfig_in, std::string* err){
	// get the text representing list of tools and their configfile version nums
	// if runconfig is given, toolsconfig for that runconfig will be used
	// else if runnum is given, toolsconfig for that runconfig will be used.
	// if neither are given the toolsconfig for the runconfig number in m_data->vars will be used
	
	int runconfigtmp;
	int* runconfig;
	if(runconfig_in!=nullptr && ((*runconfig_in)>=0) ){
		runconfig = runconfig_in;
	} else {
		runconfig = &runconfigtmp;
	}
	get_ok = GetRunConfig(*runconfig, runnum, err);
	if(not get_ok) return false;
	
	// build the query
	std::string query_string = "SELECT toolsconfig FROM runconfigs WHERE runconfig = "
	                           +pqxx::to_string(*runconfig)+" ;";
	// perform the query
	if(verbosity>v_debug){
		std::cout<<"Getting list of tools and configfile versions with query \n"<<query_string<<"\n";
	}
	get_ok = m_data->postgres.ExecuteQuery(query_string, toolsconfig);
	// FIXME replace ExecuteQuery call with Query to get better error info?
	// or perhaps better, add error info to Query method?
	if(not get_ok){
		std::string errmsg="error getting toolsconfig for runconfig "+std::to_string(*runconfig);
		std::cerr<<errmsg<<std::endl;
		if(err) *err = errmsg;
		return false;
	}
	
	return true;
}

bool PGHelper::GetToolConfig(std::string toolname, std::string& toolconfig, int* runconfig_in, int* runnum_in, std::string* err){
	// Get the tool configuration for the given tool in the current toolchain
	
	// first get the toolsconfig for the current run config
	std::string toolsconfig;
	if(verbosity>v_debug) std::cout<<"getting toolsconfig"<<std::endl;
	get_ok = GetToolsConfig(toolsconfig, runconfig_in, runnum_in, err);
	if(not get_ok) return false;
	
	// parse it into a store
	if(verbosity>v_debug) std::cout<<"parsing toolsconfig"<<std::endl;
	BoostStore toolsconfigstore;
	get_ok = ParseToolsConfig(toolsconfig, toolsconfigstore, err);
	if(not get_ok) return false;
	
	// extract the entry corresponding to the given unique identifier of this Tool instance.
	std::pair<std::string, int> configkey;
	if(verbosity>v_debug) std::cout<<"getting config version for tool instance "<<toolname<<std::endl;
	get_ok = toolsconfigstore.Get(toolname, configkey);
	if(not get_ok){
		std::string errmsg = std::string("PGHelper::GetToolConfig did not find tool '")
		                     +toolname+"' in the toolsconfig for the current runconfig!";
		std::cerr<<errmsg<<std::endl;
		if(err) *err=errmsg;
		return false;
	}
	
	// use the class name and configfile version to get the tool config
	if(verbosity>v_debug) std::cout<<"querying configuration for tool "<<configkey.first
	                               <<", version "<<configkey.second<<std::endl;
	get_ok = GetToolConfig(configkey.first, configkey.second, toolconfig, err);
	
	return get_ok;
}

bool PGHelper::GetToolConfig(std::string className, int version, std::string& toolconfig, std::string* err){
	// use the tool class name and version number to look up the Tool config
	
	std::string query_string = "SELECT contents FROM configfiles WHERE tool = "
	                           +m_data->postgres.pqxx_quote(className)
	                           +" AND version = "+std::to_string(version)+" ;";
	
	// perform the query
	if(verbosity>v_debug){
		std::cout<<"Getting tool "<<className<<" configfile version "<<version
		         <<" with query \n"<<query_string<<"\n";
	}
	get_ok = m_data->postgres.ExecuteQuery(query_string, toolconfig);
	
	if(not get_ok){
		std::string errmsg="error getting config for tool "+className
		                  +", version "+std::to_string(version);
		std::cerr<<errmsg<<std::endl;
		if(err) *err = errmsg;
		return false;
	}
	
	return true;
}

int PGHelper::InsertToolConfig(Store config, std::string toolname, std::string author, std::string description, std::string* err){
	// insert a new Tool configuration entry
	
	// each entry in the configfiles table contains:
	// a tool name,          <-| together these 2 uniquely
	// a version number,     <-| identify a configuration file
	// an author,
	// a creation timestamp,
	// a description,
	// the config file contents
	
	// Use the Store streamer to generate the json string
	std::string json_string;
	config >> json_string;
	
	// we'll allocate a new version number as a sequence of config files for this tool
	// we do this manually, for now. get the current max version number for this tool.
	std::string query_string = "SELECT max(version) FROM configfiles WHERE name = "
	                         + m_data->postgres.pqxx_quote(toolname);
	if(verbosity>v_debug){
		std::cout<<"Querying max version number for tool "<<toolname<<std::endl;
	}
	pqxx::row returnedrow;
	get_ok = m_data->postgres.Query(query_string, 2, nullptr, &returnedrow, err);
	if(not get_ok){
		std::string errmsg="error getting max version number for tool "+toolname;
		std::cerr<<errmsg<<std::endl;
		if(err) *err = errmsg;
		return false;
	}
	// else extract the result from the returned row:
	int versionnum = returnedrow[0].as<int>();
	++versionnum; // our new entry will be the next one up.
	if(verbosity>v_debug) std::cout<<"This config file will be config version number "<<versionnum<<std::endl;
	
	// creation timestamp will also be automatically generated as NOW()
	std::string created="NOW()";
	
	std::vector<std::string> fields_to_fill{"tool", "version", "author", "created", "description", "contents"};
	get_ok = m_data->postgres.Insert("configfiles", fields_to_fill, err, toolname,
	                                                                     versionnum,
	                                                                     author,
	                                                                     created,
	                                                                     description,
	                                                                     json_string
	                                                                     );
	
	return get_ok; // return if we succeeded - if not error will already be populated
}

bool PGHelper::ParseToolsConfig(std::string toolsconfig, BoostStore& configStore, std::string* err){
	std::string errmsg="errors occurred converting config version to integer for tools: ";
	std::string line;
	std::string uniqueName, className, versionNum;
	std::stringstream toolsconfigstream(toolsconfig);
	bool get_ok = true;
	while(getline(toolsconfigstream,line)){
		if(line[0]=='#') continue;
		if(line.empty()) continue;
		std::stringstream ss(line);
		ss >> uniqueName >> className >> versionNum;
		//std::cout<<uniqueName<<":"<<className<<":"<<versionNum<<std::endl;
		
		// some tools may not need any configuration variables,
		// ben has a habit of using 'NULL' in such cases
		if(versionNum=="NULL") versionNum="-1";
		
		// convert the version number to an integer
		int version;
		try {
			version = std::stoi(versionNum);
			configStore.Set(uniqueName, std::pair<std::string,int>{className, version});
		} catch (...){
			get_ok = false;
			errmsg += "'"+uniqueName+"'";
		}
	}
	if(!get_ok){
		std::cerr<<errmsg<<std::endl;
		if(err) *err=errmsg;
	}
	return get_ok;
}
