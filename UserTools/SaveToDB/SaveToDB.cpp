#include "SaveToDB.h"

SaveToDB::SaveToDB():Tool(){}


bool SaveToDB::Initialise(std::string configfile, DataModel &data){
	
	if(configfile!="")  m_variables.Initialise(configfile);
	//m_variables.Print();
	
	m_variables.Get("max_webpage_records",max_webpage_records);
	m_variables.Get("verbosity",verbosity);
	
	m_data= &data;
	
	return true;
}


bool SaveToDB::Execute(){
	
	Log("SaveToDB Executing...",v_debug,verbosity);
	
	// for each Tool in the ToolChain, execute the corresponding
	// method to record its outputs to the database
	bool get_ok = true;
	get_ok &= MarcusScheduler();
	get_ok &= BenPower();
	get_ok &= Valve();
	get_ok &= BenLED();
	get_ok &= BenSpectrometer();
	get_ok &= TraceAverage();
	get_ok &= MatthewAnalysis();
	get_ok &= SaveTraces();
	get_ok &= RoutineCalibration();  // placeholder, Tool TODO
	
	return get_ok;
}


bool SaveToDB::Finalise(){
	
	return true;
}

bool SaveToDB::MatthewAnalysis(){
	
	// see if we have new data to add to DB
	bool new_measurement;
	bool get_ok = m_data->CStore.Get("NewMeasurement",new_measurement);
	if(get_ok && new_measurement){
		
		// look up the run number
		int runnum;
		get_ok = m_data->CStore.Get("run_num",runnum);
		if(not get_ok){
			Log("SaveToDB::MatthewAnalysis no run_num in CStore!",0,0);
			return false; // we need this
		}
		
		////////////////////////////////////////////////
		// gd concentration calculation proceeds as follows:
		// 1. do dark subtraction, split into absorption and sideband regions
		// 2. fit pure water reference to this plot
		// 3. calculate absorption as log of the ratio between the scaled pure water and data
		// 4. fit the two main peaks in absorbance with gaussians
		// 5. convert difference in amplitudes to concentration using calibration data
		// we'll show results from various intermediate steps
		////////////////////////////////////////////////
		
		// 0, raw traces
		// for monitoring consistency, record some characteristics about the raw dark and led-on traces
		// for the dark trace, we histogram it, fit it with a gaussian, and record the mean and sigma
		double dark_mean, dark_sigma;
		get_ok  = m_data->CStore.Get("dark_mean",dark_mean);
		get_ok &= m_data->CStore.Get("dark_sigma",dark_sigma);
		if(not get_ok){
			Log("SaveToDB::MatthewAnalysis did not find dark trace mean & sigma in CStore!",0,0);
		} else {
			std::string dark_json = "{ \"mean\":"+std::to_string(dark_mean)
			                      + ", \"width\":"+std::to_string(dark_sigma)+"}";
			// store to database
			field_names = std::vector<std::string>{"run","timestamp","tool","name","values"};
			error_ret="";
			m_data->postgres.Insert("data",                      // table name
			                        field_names,                 // field names
			                        &error_ret,                  // error return string
			                        // variadic argument list of field values
			                        runnum,                      // run
			                        "now()",                     // timestamp
			                        "MatthewAnalysis",           // tool
			                        "darktrace_params",          // name
			                        dark_json);                  // values (jsonb)
			if(error_ret!=""){
				Log("SaveToDB::MatthewAnalysis failed to save dark trace characteristics "
				    "into database with error '"+error_ret+"'",0,0);
			}
		}
		
		// for the led-on trace, we record the max and min in the 270-280nm absorption region.
		double led_on_max, led_on_min;
		get_ok  = m_data->CStore.Get("raw_absregion_max",led_on_max);
		get_ok &= m_data->CStore.Get("raw_absregion_min",led_on_min);
		if(not get_ok){
			Log("SaveToDB::MatthewAnalysis did not find raw trace min/max in CStore!",0,0);
		} else {
			std::string rawtrace_json = "{ \"max\":"+std::to_string(led_on_max)
			                          + ", \"min\":"+std::to_string(led_on_min)+"}";
			// store to database
			field_names = std::vector<std::string>{"run","timestamp","tool","name","values"};
			error_ret="";
			m_data->postgres.Insert("data",                      // table name
				                    field_names,                 // field names
				                    &error_ret,                  // error return string
				                    // variadic argument list of field values
				                    runnum,                      // run
				                    "now()",                     // timestamp
				                    "MatthewAnalysis",           // tool
				                    "rawtrace_params",           // name
				                    rawtrace_json);              // values (jsonb)
			if(error_ret!=""){
				Log("SaveToDB::MatthewAnalysis failed to save raw trace characteristics "
					"into database with error '"+error_ret+"'",0,0);
			}
		}
		
		// 1, dark-subtracted data trace
		// -----------------------------
		intptr_t dark_subtracted_gd_p;
		get_ok = m_data->CStore.Get("dark_subtracted_gd",dark_subtracted_gd_p);
		
		// we'll indicate points inside and outside the fit range differently,
		// in order to assist with verification of the pure water fit.
		// To do this, we need to split the data into two arrays;
		// points inside the absorption region and those outside it.
		std::pair<double,double> fit_range;
		get_ok &= m_data->CStore.Get("gd_fit_range", fit_range);
		
		if(not get_ok){
			Log("SaveToDB::MatthewAnalysis did not find dark subtracted data in CStore!",0,0);
		} else {
			TGraphErrors* dark_subtracted_gd = reinterpret_cast<TGraphErrors*>(dark_subtracted_gd_p);
			
			// to store in database we need to convert to json array.
			std::string gd_data_inside_absregion =  BuildJson(dark_subtracted_gd, fit_range, true);
			std::string gd_data_outside_absregion = BuildJson(dark_subtracted_gd, fit_range, false);
			
			// store in temporary db
			field_names = std::vector<std::string>{"timestamp","name","values"};
			error_ret="";
			m_data->postgres.Insert("webpage",                   // table name
			                        field_names,                 // field names
			                        &error_ret,                  // error return string
			                        // variadic argument list of field values
			                        "now()",                     // timestamp
			                        "dark_subtracted_data_in",   // name
			                        gd_data_inside_absregion);   // values (jsonb)
			if(error_ret!=""){
				Log("SaveToDB::MatthewAnalysis failed to insert dark subtracted data withinin "
					"absorption region into database with error '"+error_ret+"'",0,0);
			}
			error_ret="";
			m_data->postgres.Insert("webpage",                   // table name
			                        field_names,                 // field names
			                        &error_ret,                  // error return string
			                        // variadic argument list of field values
			                        "now()",                     // timestamp
			                        "dark_subtracted_data_out",  // name
			                        gd_data_outside_absregion);  // values (jsonb)
			if(error_ret!=""){
				Log("SaveToDB::MatthewAnalysis failed to insert dark subtracted data outside "
					"absorption region into database with error '"+error_ret+"'",0,0);
			}
			// retain only the last N entries from the website table
			query_string = "DELETE FROM webpage WHERE name LIKE 'dark_subtracted_data_%' "
			               "AND id NOT IN (SELECT id FROM webpage WHERE "
			               "name='dark_subtracted_data_in' ORDER BY id DESC LIMIT "
			               +std::to_string(max_webpage_records)+");";
			get_ok = m_data->postgres.ExecuteQuery(query_string);
			if(not get_ok){
				Log("SaveToDB::MatthewAnalysis failed to delete old dark_subtracted_data records "
				    "from webpage table",0,0);
			}
			
		}
		
		// 2. store reference pure trace before scaling
		// for persistent db storage, just record the name of the reference data file
		std::string pure_filename;
		get_ok = m_data->CStore.Get("pure_filename", pure_filename);
		if(not get_ok){
			Log("SaveToDB::MatthewAnalysis failed to get pure reference trace filename from CStore!",0,0);
		} else {
			// store to database
			field_names = std::vector<std::string>{"run","timestamp","tool","name","values"};
			error_ret="";
			m_data->postgres.Insert("data",                      // table name
			                        field_names,                 // field names
			                        &error_ret,                  // error return string
			                        // variadic argument list of field values
			                        runnum,                      // run
			                        "now()",                     // timestamp
			                        "MatthewAnalysis",           // tool
			                        "purewater_filename",        // name
			                        pure_filename);              // values (jsonb)
			if(error_ret!=""){
				Log("SaveToDB::MatthewAnalysis failed to save pure water reference data filename "
				    "into database with error '"+error_ret+"'",0,0);
			}
		}
		
		// for website we'll temporarily store the pure water trace itself, for plotting
		TGraphErrors* dark_subtracted_pure = m_data->dark_sub_pure;
		// convert to json
		std::string dark_sub_pure = BuildJson(dark_subtracted_pure);
		// delete any existing entry so we don't keep accumulating them
		query_string = "DELETE FROM webpage WHERE name = 'dark_subtracted_pure'";
		get_ok = m_data->postgres.ExecuteQuery(query_string);
		if(not get_ok){
			Log("SaveToDB::MatthewAnalysis failed to delete existing dark_subtracted_pure record "
			    "from webpage table",0,0);
		}
		// insert a new record
		field_names = std::vector<std::string>{"timestamp","name","values"};
		error_ret="";
		m_data->postgres.Insert("webpage",
		                        field_names,
		                        &error_ret,
		                        "now()",
		                        "dark_subtracted_pure",
		                        dark_sub_pure);
		if(error_ret!=""){
			Log("SaveToDB::MatthewAnalysis failed to insert new 'dark_subtracted_pure' "
			    "record into webpage table with error "+error_ret,0,0);
		}
		
		
		// store reference pure trace after scaling to fit the Gd data in the lobes of the LED peak
		intptr_t pure_scaled_p;
		get_ok = m_data->CStore.Get("pure_scaled",pure_scaled_p);
		if(not get_ok){
			Log("SaveToDB::MatthewAnalysis failed to get scaled pure function from CStore!",0,0);
		} else {
			TGraphErrors* pure_scaled = reinterpret_cast<TGraphErrors*>(pure_scaled_p);
			// convert to json
			std::string pure_scaled_json = BuildJson(pure_scaled);
			// delete any existing entry so we don't keep accumulating them
			query_string = "DELETE FROM webpage WHERE name = 'pure_scaled'";
			get_ok = m_data->postgres.ExecuteQuery(query_string);
			if(not get_ok){
				Log("SaveToDB::MatthewAnalysis failed to delete existing 'pure_scaled' entry "
				    "from webpage table",0,0);
			}
			// insert new record in its place
			field_names = std::vector<std::string>{"timestamp","name","values"};
			error_ret="";
			m_data->postgres.Insert("webpage",                   // table name
			                        field_names,                 // field names
			                        &error_ret,                  // error return string
			                        // variadic argument list of field values
			                        "now()",                     // timestamp
			                        "pure_scaled",               // name
			                        pure_scaled_json);           // values (jsonb)
			if(error_ret!=""){
				Log("SaveToDB::MatthewAnalysis failed to insert new scaled pure data "
				    "into database with error '"+error_ret+"'",0,0);
			}
		}
		
		// get the TF1 for fit parameters and errors.
		// the TF1 is a version of the pure water reference, scaled and with a linear background added
		// i.e. it has 4 parameters; the reference scaling and y,m,c from the linear background
		TF1* pure_fct = m_data->pure_fct;
		
		// store fit parameters and errors
		double* puretogd_fitting_parameters = pure_fct->GetParameters();
		double* puretogd_fitting_errors = const_cast<double*>(pure_fct->GetParErrors());
		// convert to json
		std::string pure_fit_pars = BuildJson(puretogd_fitting_parameters,
		                                      puretogd_fitting_errors,
		                                      pure_fct->GetNpar());
		// store to db. These get stored persistently, not just temporarily for the webpage
		field_names = std::vector<std::string>{"run","timestamp","tool","name","values"};
		error_ret="";
		m_data->postgres.Insert("data",                      // table name
		                        field_names,                 // field names
		                        &error_ret,                  // error return string
		                        // variadic argument list of field values
		                        runnum,                      // run
		                        "now()",                     // timestamp
		                        "MatthewAnalysis",           // tool
		                        "pure_fit_pars",             // name
		                        pure_fit_pars);              // values (jsonb)
		if(error_ret!=""){
			Log("SaveToDB::MatthewAnalysis failed to insert new pure data fit parameters "
			    "into database with error '"+error_ret+"'",0,0);
		}
		
		// Get FitResultPtr, which contains info on goodness of fit
		intptr_t fitresptr_p;
		get_ok = m_data->CStore.Get("data_fitresptr",fitresptr_p);
		if(not get_ok){
			Log("SaveToDB::MatthewAnalysis failed to get pure water fit result from CStore!",0,0);
		} else {
			TFitResultPtr* fitresptr = reinterpret_cast<TFitResultPtr*>(fitresptr_p);
			std::string fit_status = BuildJson(*fitresptr);
			// store to database
			field_names = std::vector<std::string>{"run","timestamp","tool","name","values"};
			error_ret="";
			m_data->postgres.Insert("data",                      // table name
				                    field_names,                 // field names
				                    &error_ret,                  // error return string
				                    // variadic argument list of field values
				                    runnum,                      // run
				                    "now()",                     // timestamp
				                    "MatthewAnalysis",           // tool
				                    "pure_fit_status",           // name
				                    fit_status);                 // values (jsonb)
			if(error_ret!=""){
				Log("SaveToDB::MatthewAnalysis failed to insert new pure data fit status "
					"into database with error '"+error_ret+"'",0,0);
			}
		}
		
		// 3. absorption trace: log10 of fraction of transmitted light. Two peaks.
		intptr_t absorbance_p;
		get_ok = m_data->CStore.Get("absorbance",absorbance_p);
		if(not get_ok){
			Log("SaveToDB::MatthewAnalysis failed to get absorbance data from CStore!",0,0);
		} else {
			TGraphErrors* absorbance = reinterpret_cast<TGraphErrors*>(absorbance_p);
			// convert to json
			std::string absorbance_json = BuildJson(absorbance);
			// delete any existing entry so we don't keep accumulating them
			field_names = std::vector<std::string>{"timestamp","name","values"};
			error_ret="";
			m_data->postgres.Insert("webpage",                   // table name
			                        field_names,                 // field names
			                        &error_ret,                  // error return string
			                        // variadic argument list of field values
			                        "now()",                     // timestamp
			                        "absorbance_trace",          // name
			                        absorbance_json);            // values (jsonb)
			if(error_ret!=""){
				Log("SaveToDB::MatthewAnalysis failed to insert absorbance trace "
				    "into database with error '"+error_ret+"'",0,0);
			}
			// retain only the last N entries from the website table
			query_string = "DELETE FROM webpage WHERE name='absorbance_trace' AND id NOT IN "
			               "(SELECT id FROM webpage WHERE name='absorbance_trace' ORDER BY id DESC LIMIT "
			               +std::to_string(max_webpage_records)+");";
			get_ok = m_data->postgres.ExecuteQuery(query_string);
			if(not get_ok){
				Log("SaveToDB::MatthewAnalysis failed to delete old absorbance_trace records "
					"from webpage table",0,0);
			}
		}
		
		// 4. gaussians fit to absorption peaks
		// store parameters of the TF1s
		// (TODO store fit function formula, in case we change it?)
		intptr_t left_peak_p, right_peak_p;
		intptr_t lpf_p, rpf_p;
		get_ok  = m_data->CStore.Get("LeftPeakFitFunc",left_peak_p);
		get_ok &= m_data->CStore.Get("RightPeakFitFunc",right_peak_p);
		get_ok &= m_data->CStore.Get("LeftPeakFitResult",lpf_p);
		get_ok &= m_data->CStore.Get("RightPeakFitResult",rpf_p);
		if(not get_ok){
			Log("SaveToDB::MatthewAnalysis failed to get absorption peak fits from CStore!",0,0);
		} else {
			TF1* left_peak = reinterpret_cast<TF1*>(left_peak_p);
			TF1* right_peak = reinterpret_cast<TF1*>(right_peak_p);
			TFitResultPtr* lpf = reinterpret_cast<TFitResultPtr*>(lpf_p);
			TFitResultPtr* rpf = reinterpret_cast<TFitResultPtr*>(rpf_p);
			
			// store fit parameters and errors - left absorption peak
			// TODO refactor this to appropriate functions: 3 fits get stored, same code each time
			double* leftabspk_fitting_parameters = left_peak->GetParameters();
			double* leftabspk_fitting_errors = const_cast<double*>(left_peak->GetParErrors());
			// convert to json
			std::string leftabspk_fit_pars = BuildJson(leftabspk_fitting_parameters,
			                                           leftabspk_fitting_errors,
			                                           left_peak->GetNpar());
			// store to db. These get stored persistently, not just temporarily for the webpage
			field_names = std::vector<std::string>{"run","timestamp","tool","name","values"};
			error_ret="";
			m_data->postgres.Insert("data",                      // table name
			                        field_names,                 // field names
			                        &error_ret,                  // error return string
			                        // variadic argument list of field values
			                        runnum,                      // run
			                        "now()",                     // timestamp
			                        "MatthewAnalysis",           // tool
			                        "left_abspk_fit_pars",       // name
			                        leftabspk_fit_pars);         // values (jsonb)
			if(error_ret!=""){
				Log("SaveToDB::MatthewAnalysis failed to insert left absorption peak fit parameters "
				    "into database with error '"+error_ret+"'",0,0);
			}
			
			std::string leftpeak_fit_status = BuildJson(*lpf);
			// store to database
			field_names = std::vector<std::string>{"run","timestamp","tool","name","values"};
			error_ret="";
			m_data->postgres.Insert("data",                      // table name
			                        field_names,                 // field names
			                        &error_ret,                  // error return string
			                        // variadic argument list of field values
			                        runnum,                      // run
			                        "now()",                     // timestamp
			                        "MatthewAnalysis",           // tool
			                        "left_abspk_fit_status",     // name
			                        leftpeak_fit_status);        // values (jsonb)
			if(error_ret!=""){
				Log("SaveToDB::MatthewAnalysis failed to insert left absorption peak fit status "
				    "into database with error '"+error_ret+"'",0,0);
			}
			
			// repeat for right peak
			double* rightabspk_fitting_parameters = right_peak->GetParameters();
			double* rightabspk_fitting_errors = const_cast<double*>(right_peak->GetParErrors());
			// convert to json
			std::string rightabspk_fit_pars = BuildJson(rightabspk_fitting_parameters,
			                                            rightabspk_fitting_errors,
			                                            right_peak->GetNpar());
			// store to db. These get stored persistently, not just temporarily for the webpage
			field_names = std::vector<std::string>{"run","timestamp","tool","name","values"};
			error_ret="";
			m_data->postgres.Insert("data",                      // table name
			                        field_names,                 // field names
			                        &error_ret,                  // error return string
			                        // variadic argument list of field values
			                        runnum,                      // run
			                        "now()",                     // timestamp
			                        "MatthewAnalysis",           // tool
			                        "right_abspk_fit_pars",      // name
			                        rightabspk_fit_pars);        // values (jsonb)
			if(error_ret!=""){
				Log("SaveToDB::MatthewAnalysis failed to insert right absorption peak fit parameters "
				    "into database with error '"+error_ret+"'",0,0);
			}
			
			std::string rightpeak_fit_status = BuildJson(*rpf);
			// store to database
			field_names = std::vector<std::string>{"run","timestamp","tool","name","values"};
			error_ret="";
			m_data->postgres.Insert("data",                      // table name
			                        field_names,                 // field names
			                        &error_ret,                  // error return string
			                        // variadic argument list of field values
			                        runnum,                      // run
			                        "now()",                     // timestamp
			                        "MatthewAnalysis",           // tool
			                        "right_abspk_fit_status",    // name
			                        rightpeak_fit_status);       // values (jsonb)
			if(error_ret!=""){
				Log("SaveToDB::MatthewAnalysis failed to insert right absorption peak fit status "
				    "into database with error '"+error_ret+"'",0,0);
			}
			// we'll leave the user to re-calculate the difference in heights
			// from the parameters from the 2 gaussian fits if they desire.
		}
		
		// 6. convert difference in amplitudes to concentration
		// since we'll re-use the calibration curve mapping peak height difference to concentration
		// for many measurements, only store the calibration version used for this measurement
		int calib_version;
		get_ok = m_data->CStore.Get("calib_version",calib_version);
		if(not get_ok){
			Log("SaveToDB::MatthewAnalysis failed to retrieve calibration version from CStore",0,0);
		} else {
			// FIXME calling 'psql -c "INSERT INTO mytable ( jsonb_field ) VALUES ( 3 )";
			// produces an error 'column "jsonb_field" is of type jsonb but expression is of type integer
			// whereas using "... VALUES ( '3' ) ..." works. Not sure how this interacts with
			// exec_params (which underlies Postgres::Insert); we may need to cast to string
			// store to database
			field_names = std::vector<std::string>{"run","timestamp","tool","name","values"};
			error_ret="";
			m_data->postgres.Insert("data",                      // table name
			                        field_names,                 // field names
			                        &error_ret,                  // error return string
			                        // variadic argument list of field values
			                        runnum,                      // run
			                        "now()",                     // timestamp
			                        "MatthewAnalysis",           // tool
			                        "calibration_version",       // name
			                        calib_version);              // values (jsonb)
			if(error_ret!=""){
				Log("SaveToDB::MatthewAnalysis failed to insert calibration curve version "
				    "into database with error '"+error_ret+"'",0,0);
			}
		}
		
		// store calculated concentration
		std::pair<double,double> gdconcentration = m_data->concs_and_errs.back();
		std::string gdconcentration_string = "{ \"val\":"+std::to_string(gdconcentration.first)
		                                   + ", \"err\":"+std::to_string(gdconcentration.second)+" }";
		field_names = std::vector<std::string>{"run","timestamp","tool","name","values"};
		error_ret="";
		m_data->postgres.Insert("data",                      // table name
		                        field_names,                 // field names
		                        &error_ret,                  // error return string
		                        // variadic argument list of field values
		                        runnum,                      // run
		                        "now()",                     // timestamp
		                        "MatthewAnalysis",           // tool
		                        "gdconcentration",           // name
		                        gdconcentration_string);     // values (jsonb)
		if(error_ret!=""){
			Log("SaveToDB::MatthewAnalysis failed to insert calculated concentration "
			    "into database with error '"+error_ret+"'",0,0);
		}
		
		// TODO draw lines on graph where this crosses: lookup line to lead eye
		
	} else {
		get_ok = true;   // no new measurement, nothing to save
	}
	
	return get_ok;
	
}

bool SaveToDB::BenPower(){
	bool all_ok = true;
	
	std::string Power="OFF";   // or "ON"
	m_data->CStore.Get("Power",Power);
	Power = "{\"status\": \""+Power+"\"}";
	
	// store to database
	field_names = std::vector<std::string>{"timestamp","values"};
	criterions = std::vector<std::string>{"name"};
	comparators = std::vector<char>{'='};
	error_ret="";
	m_data->postgres.Update("webpage",                   // table name
	                        field_names,                 // field names
	                        criterions,                  // fields we're applying SELECT on to match
	                        comparators,                 // comparison operator required by match
	                        &error_ret,                  // error return string
	                        // variadic argument list of new field values
	                        "now()",                     // timestamp
	                        Power,                       // new values (jsonb)
	                        // variadic argument list of criterion values
	                        "powerstate");               // name
	if(error_ret!=""){
		Log("SaveToDB::BenPower failed to update power state "
		    "in database with error '"+error_ret+"'",0,0);
		all_ok = false;
	}
	
	return all_ok;
}

bool SaveToDB::Valve(){
	bool all_ok = true;
	
	std::string valve="CLOSED";  // or "OPEN"
	m_data->CStore.Get("Valve",valve);
	if(valve=="CLOSE") valve="CLOSED"; // grammar.
	valve = "{\"status\": \""+valve+"\"}";
	
	// store to database
	field_names = std::vector<std::string>{"timestamp","values"};
	criterions = std::vector<std::string>{"name"};
	comparators = std::vector<char>{'='};
	error_ret="";
	m_data->postgres.Update("webpage",                   // table name
	                        field_names,                 // field names
	                        criterions,                  // fields we're applying SELECT on to match
	                        comparators,                 // comparison operator required by match
	                        &error_ret,                  // error return string
	                        // variadic argument list of new field values
	                        "now()",                     // timestamp
	                        valve,                       // new values (jsonb)
	                        // variadic argument list of criterion values
	                        "valvestate");               // name
	if(error_ret!=""){
		Log("SaveToDB::Valve failed to update valve state "
		    "in database with error '"+error_ret+"'",0,0);
		all_ok = false;
	}
	
	return all_ok;
}

bool SaveToDB::BenLED(){
	
	bool all_ok = true;
	
	// handle for PWM control board. If present and !=0, we have comms
	int pwm_comms_ok=0;
	m_data->CStore.Get("PWMBoardHandle",pwm_comms_ok);
	// store to database
	field_names = std::vector<std::string>{"timestamp","values"};
	criterions = std::vector<std::string>{"name"};
	comparators = std::vector<char>{'='};
	error_ret="";
	m_data->postgres.Update("webpage",                   // table name
	                        field_names,                 // field names
	                        criterions,                  // fields we're applying SELECT on to match
	                        comparators,                 // comparison operator required by match
	                        &error_ret,                  // error return string
	                        // variadic argument list of new field values
	                        "now()",                     // timestamp
	                        pwm_comms_ok,                // new values (jsonb)
	                        // variadic argument list of criterion values
	                        "pwm_connected");            // name
	if(error_ret!=""){
		Log("SaveToDB::BenLED failed to update PWM comms state "
		    "in database with error '"+error_ret+"'",0,0);
		all_ok = false;
	}
	
	std::map<std::string,bool> ledStatuses;
	m_data->CStore.Get("ledStatuses",ledStatuses);
	// convert to json to insert into DB
	std::string ledstatusstring="{ ";
	for(auto&& ledStatus : ledStatuses){
		ledstatusstring += "\""+ledStatus.first+"\":"+std::to_string(ledStatus.second)+",";
	}
	// replace trailing ',' with closing '}'
	ledstatusstring.back()='}';
	// store to database
	field_names = std::vector<std::string>{"timestamp","values"};
	criterions = std::vector<std::string>{"name"};
	comparators = std::vector<char>{'='};
	error_ret="";
	m_data->postgres.Update("webpage",                   // table name
	                        field_names,                 // field names
	                        criterions,                  // fields we're applying SELECT on to match
	                        comparators,                 // comparison operator required by match
	                        &error_ret,                  // error return string
	                        // variadic argument list of new field values
	                        "now()",                     // timestamp
	                        ledstatusstring,             // new values (jsonb)
	                        // variadic argument list of criterion values
	                        "ledStatuses");              // name
	if(error_ret!=""){
		Log("SaveToDB::BenLED failed to update LED states "
		    "in database with error '"+error_ret+"'",0,0);
		all_ok = false;
	}
	
	return all_ok;
	
}

bool SaveToDB::BenSpectrometer(){
	bool all_ok = true;
	
	// get connection status
	bool connected=false;
	m_data->CStore.Get("SpectrometerConnected",connected);
	
	// store to database
	field_names = std::vector<std::string>{"timestamp","values"};
	criterions = std::vector<std::string>{"name"};
	comparators = std::vector<char>{'='};
	error_ret="";
	m_data->postgres.Update("webpage",                   // table name
	                        field_names,                 // field names
	                        criterions,                  // fields we're applying SELECT on to match
	                        comparators,                 // comparison operator required by match
	                        &error_ret,                  // error return string
	                        // variadic argument list of new field values
	                        "now()",                     // timestamp
	                        connected,                   // new values (jsonb)
	                        // variadic argument list of criterion values
	                        "spectrometer_connected");   // name
	if(error_ret!=""){
		Log("SaveToDB::BenSpectrometer failed to update spectrometer connection status "
		    "in database with error '"+error_ret+"'",0,0);
		all_ok = false;
	}
	
	return all_ok;
	
}

bool SaveToDB::TraceAverage(){
	bool all_ok = true;
	
	bool new_data=false;
	bool get_ok = m_data->CStore.Get("NewData",new_data);
	if(get_ok && new_data){
		
		// Store latest trace (averaged over spectrometer acquisitions) in DataModel TTree
		
		// get ttree holding latest trace
		std::string name="test";
		m_data->CStore.Get("Trace",name);
		TTree* tree=m_data->GetTTree(name);
		
		// set up branches and retrieve latest entry
		std::vector<double> value;       std::vector<double>* a=&value;
		std::vector<double> error;       std::vector<double>* b=&error;
		std::vector<double> wavelength;  std::vector<double>* c=&wavelength;
		bool err=false;
		err |= (tree->SetBranchAddress("value", &a) < 0);
		err |= (tree->SetBranchAddress("error", &b) < 0);
		err |= (tree->SetBranchAddress("wavelength", &c) < 0);
		err |= ((tree->GetEntry(tree->GetEntries()-1)) <= 0);
		if(err){
			std::stringstream ss;
			ss << "SaveToDB::TraceAverage failed to get data for latest trace from TTree "
			   << name <<" at " << tree;
			Log(ss.str(),0,0);
			//return false;
		}
		// convert 3 vectors to json
		std::string trace_data = "{ \"yvals\":[ ";
		for(int i=0; i<value.size(); ++i){
			trace_data += std::to_string(value.at(i))+",";
		}
		trace_data.back()=']';
		trace_data += ", \"yerrs\":[ ";
		for(int i=0; i<error.size(); ++i){
			trace_data += std::to_string(error.at(i))+",";
		}
		trace_data.back()=']';
		trace_data += ", \"xvals\":[ ";
		for(int i=0; i<wavelength.size(); ++i){
			trace_data += std::to_string(wavelength.at(i))+",";
		}
		trace_data.back()=']';
		trace_data += "}";
		
		// store to database
		field_names = std::vector<std::string>{"timestamp","values"};
		criterions = std::vector<std::string>{"name"};
		comparators = std::vector<char>{'='};
		error_ret="";
		m_data->postgres.Update("webpage",                   // table name
		                        field_names,                 // field names
		                        criterions,                  // fields we're applying SELECT on to match
		                        comparators,                 // comparison operator required by match
		                        &error_ret,                  // error return string
		                        // variadic argument list of new field values
		                        "now()",                     // timestamp
		                        trace_data,                  // new values (jsonb)
		                        // variadic argument list of criterion values
		                        "last_trace");               // name
		if(error_ret!=""){
			Log("SaveToDB::TraceAverage failed to update last trace data "
			    "in database with error '"+error_ret+"'",0,0);
			all_ok = false;
		}
		
		// N.B. last trace is saved as a png in:
		// "/home/pi/WebServer/html/images/last.png"
	}
	
	return all_ok;
	
}

bool SaveToDB::SaveTraces(){
	bool all_ok = true;
	
	// get last filename to be written to with SaveTraces tool
	std::string name;
	m_data->CStore.Get("Filename",name);
	name = "{\"filename\":\""+name+"\"}";
	
	// store to database
	field_names = std::vector<std::string>{"timestamp","values"};
	criterions = std::vector<std::string>{"name"};
	comparators = std::vector<char>{'='};
	error_ret="";
	m_data->postgres.Update("webpage",                   // table name
	                        field_names,                 // field names
	                        criterions,                  // fields we're applying SELECT on to match
	                        comparators,                 // comparison operator required by match
	                        &error_ret,                  // error return string
	                        // variadic argument list of new field values
	                        "now()",                     // timestamp
	                        name,                        // new values (jsonb)
	                        // variadic argument list of criterion values
	                        "output_filename");          // name
	if(error_ret!=""){
		Log("SaveToDB::SaveTraces failed to update active filename "
		    "in database with error '"+error_ret+"'",0,0);
		all_ok = false;
	}
	
	return all_ok;
}

bool SaveToDB::MarcusScheduler(){
	
	bool all_ok = true;
	
	std::vector<std::string> commands;
	int current_command;
	int command_step;
	std::vector<int> loop_counts;
	m_data->CStore.Get("MarcusSchedulerCommands",commands);
	m_data->CStore.Get("MarcusSchedulerCurrentCommand",current_command);
	m_data->CStore.Get("MarcusSchedulerCommandStep",command_step);
	m_data->CStore.Get("MarcusSchedulerLoopCounts",loop_counts);
	
	// TODO scan commands and add loop counters
	
	// convert to json
	std::string scheduler_commands = BuildJson(commands);
	// store to database
	field_names = std::vector<std::string>{"timestamp","values"};
	criterions = std::vector<std::string>{"name"};
	comparators = std::vector<char>{'='};
	error_ret="";
	m_data->postgres.Update("webpage",                   // table name
	                        field_names,                 // field names
	                        criterions,                  // fields we're applying SELECT on to match
	                        comparators,                 // comparison operator required by match
	                        &error_ret,                  // error return string
	                        // variadic argument list of new field values
	                        "now()",                     // timestamp
	                        scheduler_commands,          // new values (jsonb)
	                        // variadic argument list of criterion values
	                        "scheduler_commands");       // name
	if(error_ret!=""){
		Log("SaveToDB::MarcusScheduler failed to update scheduler commands "
		    "in database with error '"+error_ret+"'",0,0);
		all_ok = false;
	}
	
	std::string loop_counts_json = BuildJson(loop_counts.data(), loop_counts.size());
	
	// convert to json
	std::string scheduler_progress = "{ \"current_command\":"+std::to_string(current_command)
	                               + ", \"command_step\":"+std::to_string(command_step)
	                               + ", \"loop_counts\":"+loop_counts_json+"}";
	// store to database
	field_names = std::vector<std::string>{"timestamp","values"};
	criterions = std::vector<std::string>{"name"};
	comparators = std::vector<char>{'='};
	error_ret="";
	m_data->postgres.Update("webpage",                   // table name
	                        field_names,                 // field names
	                        criterions,                  // fields we're applying SELECT on to match
	                        comparators,                 // comparison operator required by match
	                        &error_ret,                  // error return string
	                        // variadic argument list of new field values
	                        "now()",                     // timestamp
	                        scheduler_progress,          // new values (jsonb)
	                        // variadic argument list of criterion values
	                        "scheduler_progress");       // name
	if(error_ret!=""){
		Log("SaveToDB::MarcusScheduler failed to update scheduler progress "
		    "in database with error '"+error_ret+"'",0,0);
		all_ok = false;
	}
	
	return all_ok;
}

bool SaveToDB::RoutineCalibration(){
//	TODO
//		peak260	{gaussian mean,amp,sigma}
//		peak275	{gaussian mean,amp,sigma}
//		peak385	{gaussian mean,amp,sigma}
//		peakR	{gaussian mean,amp,sigma}
//		peakG	{gaussian mean,amp,sigma}
//		peakB	{gaussian mean,amp,sigma}
//		peakW	???
	return true;
}

// helper functions for converting data to JSON

std::string SaveToDB::BuildJson(TGraphErrors* gr, std::pair<double, double> range, bool inside_data){
	// convert TGraphErrors to json object, but selecting data only inside or outside a given range
	int npoints = gr->GetN();
	double* xvals = gr->GetX();
	double* yvals = gr->GetY();
	double* xerrs = gr->GetEX();
	double* yerrs = gr->GetEY();
	
	std::string xarr="[";
	std::string yarr="[";
	std::string xerrarr="[";
	std::string yerrarr="[";
	
	for(int i=0; i<npoints; ++i){
		bool in_range = (xvals[i] < range.first || xvals[i] > range.second);
		if((in_range && inside_data) || (!in_range && !inside_data)){
			xarr+=std::to_string(xvals[i])+",";
			yarr+=std::to_string(yvals[i])+",";
			xerrarr+=std::to_string(xerrs[i])+",";
			yerrarr+=std::to_string(yerrs[i])+",";
		}
	}
	// pop trailing , and replace with closing ]
	xarr.pop_back(); xarr+= ']';
	yarr.pop_back(); yarr+= ']';
	xerrarr.pop_back(); xerrarr+=']';
	yerrarr.pop_back(); yerrarr+=']';
	
	std::string jsonarr = "{\"xvals\":"+xarr+",\"yvals\":"+yarr
	                      +",\"xerrs\":"+xerrarr+",\"yerrs\":"+yerrarr+"}";
	return jsonarr;
}

std::string SaveToDB::BuildJson(TGraphErrors* gr){
	int npoints = gr->GetN();
	double* xvals = gr->GetX();
	double* yvals = gr->GetY();
	double* xerrs = gr->GetEX();
	double* yerrs = gr->GetEY();
	std::string jsonstring = "{\"xvals\":" + BuildJson(xvals, npoints)
	                       + ",\"yvals\":" + BuildJson(yvals, npoints)
	                       + ",\"xerrs\":" + BuildJson(xerrs, npoints)
	                       + ",\"yerrs\":" + BuildJson(yerrs, npoints)+"}";
	return jsonstring;
}

std::string SaveToDB::BuildJson(TGraph* gr){
	int npoints = gr->GetN();
	double* xvals = gr->GetX();
	double* yvals = gr->GetY();
	std::string jsonstring = "{\"xvals\":" + BuildJson(xvals, npoints)
	                       + ",\"yvals\":" + BuildJson(yvals, npoints)+"}";
	return jsonstring;
}

std::string SaveToDB::BuildJson(double* arr, double* err, int n){
	std::string jsonstring = "{\"values\":"+BuildJson(arr,n)
	                         +", \"errors\":"+BuildJson(err,n)+"}";
	return jsonstring;
}

std::string SaveToDB::BuildJson(double* arr, int n){
	std::string jsonstring = "[";
	for(int i=0; i<n; ++i){
		if(i>0) jsonstring+=",";
		jsonstring+=std::to_string(arr[n]);
	}
	jsonstring+="]";
	return jsonstring;
}

std::string SaveToDB::BuildJson(int* arr, int n){
	std::string jsonstring = "[";
	for(int i=0; i<n; ++i){
		if(i>0) jsonstring+=",";
		jsonstring+=std::to_string(arr[n]);
	}
	jsonstring+="]";
	return jsonstring;
}

std::string SaveToDB::BuildJson(std::vector<std::string> arr){
	std::string jsonstring = "[";
	for(int i=0; i<arr.size(); ++i){
		if(i>0) jsonstring+=",";
		jsonstring+= "\""+arr[i]+"\"";
	}
	jsonstring+="]";
	return jsonstring;
}

std::string SaveToDB::BuildJson(TFitResultPtr& fitresptr){
	// TODO better estimates of goodness of fit and resulting uncertainty ?
	std::string fit_status = std::string("{\"fitValid\":") + ((fitresptr->IsValid()) ? "1, " : "0, ")
	                      +  std::string("\"fitEmpty\":")  + ((fitresptr->IsEmpty()) ? "1, " : "0, ")
	                      +  std::string("\"fitStatus\":") + std::to_string(fitresptr->Status())+", "
	                      +  std::string("\"fitNDF\":")    + std::to_string(fitresptr->Ndf())+", "
	                      +  std::string("\"fitChi2\":")   + std::to_string(fitresptr->Chi2())+", "
	                      +  std::string("\"fitProb\":")   + std::to_string(fitresptr->Prob())+"}";
	return fit_status;
}

std::string SaveToDB::CastJsonb(std::string& in){
	return std::string(std::string("'") + in + "'::jsonb");
}

std::string SaveToDB::CastJsonb(int in){
	return std::string(std::string("'") + std::to_string(in) + "'::jsonb");
}
