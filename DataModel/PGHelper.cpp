#include "PGHelper.h"
#include "DataModel.h"

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
	
	// first we need a run number. see if we're given one
	if(runnum_in==nullptr || *runnum_in<0){
		// not given one. Assume the latest run in the rundb
		if(verbosity>v_debug) std::cout<<"No run number given, looking up latest run"<<std::endl;
		get_ok = GetCurrentRun(*runnum_in, &runconfig, err);
		// this gets the runconfig at the same time, and will populate any error, so we're done.
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
	// get the json representing list of tools and their configfile version nums
	// if runconfig is given, toolsconfig for that runconfig will be used
	// else if runnum is given, toolsconfig for that runconfig will be used
	// else toolsconfig for the runconfig for the latest runnum in rundb will be used
	
	int runconfigtmp;
	int* runconfig = (runconfig_in!=nullptr || *runconfig_in<0 ) ? runconfig_in : &runconfigtmp;
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


bool PGHelper::GetToolConfig(std::string toolname, std::string& toolconfig, int* versionnum_in, int* runconfig, int* runnum, std::string* err){
	
	// to uniquely identify a config file within the configfiles table
	// we need to provide the tool name, and the config file version number.
	// if no version number is given, a run config or number may be given instead,
	// in which case the corresponding version will be looked up.
	// if none of the above is given, the latest run will be assumed.
	
	int versionnumtmp=-1;
	int* versionnum = (versionnum_in || *versionnum_in<0) ? versionnum_in : &versionnumtmp;
	if(*versionnum<0){
		if(verbosity>v_debug) std::cout<<"No version number for requested Tool config"<<std::endl;
		
		std::string toolsconfig;
		get_ok = GetToolsConfig(toolsconfig, runnum, runconfig, err);
		if(not get_ok) return false;
		
		// the toolsconfig is a json map of tool names to version numbers
		// we use these together to look up the tool config file in the configfiles table.
		// first, convert the toolsconfig file to a Store for parsing
		if(verbosity>v_debug) std::cout<<"parsing toolsconfig json"<<std::endl;
		Store toolsconfigstore;
		toolsconfigstore.JsonParser(toolsconfig);
		
		// try to get the configfile version number for our specified tool
		if(not toolsconfigstore.Get(toolname,*versionnum)){
			std::string errmsg = "PGHelper::GetToolConfig error! Tool "+toolname
			                     +" is not in the list of tools for this run config!";
			std::cerr<<errmsg<<std::endl;
			if(err) *err=errmsg;
			return false;
		}
		// sanity check
		if(*versionnum<0){
			std::string errmsg = "PGHelper::GetToolConfig error! invalid version number "
			                     +std::to_string(*versionnum);
			std::cerr<<errmsg<<std::endl;
			if(err) *err=errmsg;
			return false;
		}
	} // otherwise we were given a version number
	
	// use it to look up the Tool config
	std::string query_string = "SELECT contents FROM configfiles WHERE tool = "
	                           +m_data->postgres.pqxx_quote_name(toolname)
	                           +" AND version = "+pqxx::to_string(*versionnum)+" ;";
	// perform the query
	if(verbosity>v_debug){
		std::cout<<"Getting tool "<<toolname<<" configfile version "<<(*versionnum)
		         <<" with query \n"<<query_string<<"\n";
	}
	get_ok = m_data->postgres.ExecuteQuery(query_string, toolconfig);
	if(not get_ok){
		std::string errmsg="error getting config for tool "+toolname
		                  +", version "+std::to_string(*versionnum);
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
