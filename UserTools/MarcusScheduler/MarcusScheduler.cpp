#include "MarcusScheduler.h"

MarcusScheduler::MarcusScheduler():Tool(){}

// ««-------------- ≪ °◇◆◇° ≫ --------------»»

bool MarcusScheduler::Initialise(std::string configfile, DataModel &data){
	
	if(configfile!="")  m_variables.Initialise(configfile);
	//m_variables.Print();
	
	m_data= &data;
	
	m_variables.Get("verbosity",verbosity);
	
	// check we have an input file specifying the sequence of commands to be run
	if(!m_variables.Get("command_file",command_file)){
		Log("MarcusScheduler::Initialise - no command_file specified in config file!",v_error,verbosity);
		return false;
	}
	break_loop_flagfile_name = "UserTools/MarcusScheduler/breakloop";
	m_variables.Get("break_loop_flagfile",break_loop_flagfile_name);
	
	// read command file and transfer to internal vector of commands
	ReadCommandFile();
	// put set of commands into DataModel for display on website by later tool
	m_data->CStore.Set("MarcusSchedulerCommands",commands);
	
	// if looping, do we want to overwrite each iteration's output file,
	// or append the iteration number to create a unique filename for each iteration?
	// TODO this need enabling in SaveTraces
	m_variables.Get("overwrite_saves",overwrite_saves);
	
	// initialise the starting point.
	
	// 'current_command' is the number of command in the command vector currently being run.
	current_command=-1;
	
	// some input commands require several Execute loops
	// (e.g. a 'measure 275' command requires steps to turn on the LEDs, take the measurement,
	// and turn the LEDs off again).
	// In these cases 'command_step' indexes the steps through the current command.
	command_step=0;
	
	return true;
}

// ««-------------- ≪ °◇◆◇° ≫ --------------»»

bool MarcusScheduler::Execute(){
	
	Log("MarcusScheduler Executing...",v_debug,verbosity);
	
	// update current commandfile line number in datamodel for display on website
	m_data->CStore.Set("MarcusSchedulerCurrentCommand",current_command);
	m_data->CStore.Set("MarcusSchedulerCommandStep",command_step);
	// also note the current execution status of any ongoing loops
	m_data->CStore.Set("MarcusSchedulerLoopStarts",loop_starts);
	m_data->CStore.Set("MarcusSchedulerLoopCounts",loop_counts);
	
	// everything in a try-catch loop. Mostly for debugging, but helps
	// catch situations such as running off the end of vectors,
	// and provides a more helpful debug print
	try{
		
		// startup state - do not start the command process until the user types 'Start'
		if(current_command==-1){
			// request input from user
			MainMenu();
		}
		
		// if we've performed all the automation steps, return to the main menu
		else if(current_command==commands.size()){
			Log("Reached end of command list, returning to main menu",v_message,verbosity);
			PutSystemInSafeState();
			current_command=-1;
		}
		
		// any other value of current_command represents the index of the next
		// command to execute in the command vector
		else {
			
			// get the command being executed.
			std::string the_command = commands.at(current_command);
			
			// print, for tracking progress
			Log(std::string("Processing command ")+std::to_string(current_command)
				+" step "+std::to_string(command_step),v_debug,verbosity);
			Log(std::string("current command is '")+the_command+"'",v_debug,verbosity);
			
			// to allow infinite loops we support the presence of a 'loop' command.
			// a 'start_loop' line in the command file marks the beginning of a loop.
			// a 'loop' line then indicates the loop point. On encountering the loop point,
			// processing will return to the first command after the 'start_loop' line.
			
			// We would also like a mechanism to break from such a loop.
			// we'll do this by checking for existence of a flag file.
			// check for that file now, and advance the current_command if found.
			check_break_loop(the_command);
			
			// otherwise, perform whatever actions are necessary for this command
			ProcessCommand(the_command);
			
		}
	}
	catch(std::exception& e){
		// print warning
		std::string logmessage = std::string("MarcusScheduler::Execute - caught exception ")
		    +e.what()+" during command "+std::to_string(current_command)+" of "
		    +std::to_string(commands.size())+" at step "+std::to_string(command_step);
		if(current_command<commands.size()){
			logmessage += "Corresponding command is: "+commands.at(current_command);
		}
		Log(logmessage,v_error,verbosity);
		std::cerr<<"Aborting this command!"<<std::endl;
		
		// ensure system is in a safe state - turn off all LEDs, close the pump valves
		PutSystemInSafeState();
		
		// advance to the next command
		command_step=0;
		++current_command;
	}
	
	// for the GracefulStop tool to be able to terminate infinite loops without interrupting
	// a mutli-loop measurement process, it needs to know when we are in the middle of a measurement
	m_data->CStore.Set("command_step",std::to_string(command_step));
	
	return true;
	
}

// ««-------------- ≪ °◇◆◇° ≫ --------------»»

bool MarcusScheduler::Finalise(){
	
	commands.clear();
	
	return true;
}

// ««-------------- ≪ °◇◆◇° ≫ --------------»»

void MarcusScheduler::ReadCommandFile(){
	// read the input file, storing the sequence of commands into a vector
	std::string line;
	std::ifstream myfile (command_file);
	if (myfile.is_open()){
		while(getline(myfile,line)){
			if(line.size()==0) continue;
			if(line[0]=='#') continue;
			commands.push_back(line);
			// note any loops- if present, we'll add loop indices to output filenames
			if(line.substr(0,line.find(' '))=="loop") looping=true;
		}
		myfile.close();
	} else {
		Log("MarcusScheduler::ReadCommandFile - Unable to open file: "+command_file,v_error,verbosity);
	}
}

// ««-------------- ≪ °◇◆◇° ≫ --------------»»

void MarcusScheduler::MainMenu(){
	// Main Menu - propt user for action
	std::string command;
	std::cout<<"Type Start to power up, Stop to power down, or anything else to begin automation: "<<std::endl;
	
	// read input from user
	std::cin>>command;
	
	// determine action
	if(command=="Start"){
		// bring power up
		std::string tmp("{\"Auto\":\"Stop\",\"Power\":\"ON\"}");
		m_data->CStore.JsonParser(tmp);
	
	} else if(command=="Stop"){
		// ensure power is off
		std::string tmp("{\"Auto\":\"Stop\",\"Power\":\"OFF\"}");
		m_data->CStore.JsonParser(tmp);
	
	} else if(command=="Quit"){
		std::string tmp("{\"Auto\":\"Stop\",\"Power\":\"OFF\"}");
		m_data->CStore.JsonParser(tmp);
		m_data->vars.Set("StopLoop",1);
	
	} else if(command!=""){
		// on anything else, load the first command to begin automation
		current_command=0;
	}
}

// ««-------------- ≪ °◇◆◇° ≫ --------------»»

void MarcusScheduler::PutSystemInSafeState(){
	// ensure all the LEDs are off and valves are closed
	std::string json_string = "{\"R\":\"0\",\"G\":\"0\",\"B\":\"0\",\"White\":\"0\",\"385\":\"0\",\"260\":\"0\",\"275\":\"0\",\"LED\":\"Change\",\"Valve\":\"CLOSE\"}";
	m_data->CStore.JsonParser(json_string);
}

// ««-------------- ≪ °◇◆◇° ≫ --------------»»

bool MarcusScheduler::check_break_loop(std::string& the_command){
	// check whether we need to exit this loop
	// this is indicated by the presence of a 'flag file' created by the user.
	// if such a file exists, we break the current loop. The flag file is then removed.
	
	// to account for the possibility of nested loops, the user may write a value
	// into the flag file. This indicates the number of nested loops to break out of.
	/* e.g. consider the following
	1.	loop_1:
	2.		loop_2:
	3.				command_a
	4.				command_b:   << we are here.
	5.		end_loop_2
	6.		command_c
	7.	end_loop_1
	8.	command_d
	*/
	// if the user writes nothing, or a value of '1', to the break file,
	// the inner loop will be broken out of, but the outer loop will continue
	// - i.e. we will jump to line 6. For a value of '2' we would jump to line 8. etc.
	
	int break_loops = check_for_break_file();
	
	for(int i=0; i<break_loops; ++i){
		// the flag file exists. we should break our current loop.
		
		// some commands require several Execute() calls.
		// We should not interrupt the system in the middle of a command.
		if(command_step==0){
			
			// to break the loop we will jump to the first command after the loop point
			// scan forward from our current command to find the loop point
			std::string a_command=commands.at(current_command);
			int loop_point=current_command;
			
			while(loop_point<(commands.size()-1) &&
			      a_command.substr(0,a_command.find(' '))!="loop"){
				a_command = commands.at(++loop_point);
			}
			
			// if we hit the end of the command list...
			if(loop_point==(commands.size()-1)){
				
				// check if the last command is the loop point
				if(a_command.substr(0,a_command.find(' '))=="loop"){
					// if so, no further actions beyond the loop
					Log("No further actions beyond loop point. "
					    "Returning to Main Menu",v_message,verbosity);
					
					// ensure all the LEDs are off and valves are closed
					PutSystemInSafeState();
					
					// return to the main menu.
					current_command=-1;
					
					// avoid triggering anything else in this Execute()
					the_command="dummy";
					
					break; // no further loops to break
					
				} else {
					// if we scanned to the end of the command list
					// but didn't find a 'loop' command, just ignore the flag file
					Log("MarcusScheduler::check_break_loop - Did not find a 'loop' command, "
					    "ignoring break loop",v_warning,verbosity);
					std::string syscmd="rm -f "+break_loop_flagfile_name;
					system(syscmd.c_str());
					
					return false;
				}
				
			// if we found a 'loop' command and there are other commands after it,
			// continue from the next command after the loop point
			} else {
				Log(std::string("loop point at command ")+std::to_string(loop_point)
				         +" of "+std::to_string(commands.size())+" commands, "
				          "advancing to command "+std::to_string(++loop_point),v_message,verbosity);
				
				current_command=loop_point;
				std::string syscmd="rm -f "+break_loop_flagfile_name;
				system(syscmd.c_str());
				the_command="dummy"; // skip the action we would have taken this call
				
				// remove this loop from our loop tracking vectors
				loop_starts.pop_back();
				loop_iterations.pop_back();
				loop_counts.pop_back();
			}
			
		} // else hold off breaking loop until the current measurement completes
		
	} // loop over numer of nested levels to break out from
	
	return (break_loops>0);
}

// :--------:

int MarcusScheduler::check_for_break_file(){
	// try to open the break file
	std::ifstream fin(break_loop_flagfile_name.c_str());
	if(not (fin.is_open() && fin.good())){
		// no break file, continue as normal
		return 0;
	}
	
	// otherwise the file may be empty, or it may contain a single number,
	// denoting how many layers of nested loops to break out from.
	// Try to read it.
	std::string Line;
	if(getline(fin, Line)){
		// we read a line. try to convert to number
		try{
			return std::stoi(Line);
		}
		catch(...){
			// failed to get a number - assume break just innermost loop
			return 1;
		}
	}
	// else an empty file - assume break just innermost loop
	return 1;
}

// ««-------------- ≪ °◇◆◇° ≫ --------------»»

void MarcusScheduler::ProcessCommand(std::string& the_command){
	// Queue up the next action, based on the current command and, if applicable,
	// which step we're at in carrying it out.
	
	if(the_command.substr(0,4)=="wait"){
		// wait for a specified time
		DoWait(the_command);
		
	} else if(the_command.substr(0,7)=="measure"){
		// take a spectrometer measurement
		DoMeasure(the_command);
		
	} else if(the_command.substr(0,7) == "analyse"){
		// calculate transparency from a given LED trace
		DoAnalyse(the_command);
		
	} else if(the_command.substr(0,4)=="save"){
		// save traces to file
		DoSave(the_command);
		
	} else if(the_command.substr(0,5)=="valve"){
		// open or close the inlet/outlet valves
		DoValves(the_command);
		
	} else if(the_command.substr(0,10)=="start_loop"){
		// the start of a section to loop
		StartLoop(the_command);
		
	} else if(the_command.substr(0,4)=="loop"){
		// the end of a section to loop.
		EndLoop(the_command);
		
	} else if(the_command.substr(0,5)=="dummy"){
		// dummy command, do nothing
		
	} else if(the_command.substr(0,4)=="quit"){
		// shut the system down and terminate the program
		std::string json_string("{\"Auto\":\"Stop\",\"Power\":\"OFF\"}");
		m_data->CStore.JsonParser(json_string);
		m_data->vars.Set("StopLoop",1);
		
	} else if(the_command[0]=='{'){
		// json formatted commands; directly insert into the CStore
		m_data->CStore.JsonParser(the_command);
		++current_command;
		
	} else {
		// unrecognised command string
		Log("MarcusScheduler::ProcessCommand - Unknown command action: "+the_command,v_error,verbosity);
		++current_command;
	}
	
}

// ««-------------- ≪ °◇◆◇° ≫ --------------»»

void MarcusScheduler::DoWait(std::string the_command){
	// strip off the 'wait' to get at the time
	std::string wait_time = the_command.substr(5,std::string::npos);
	
	// strip off any trailing comment
	wait_time = wait_time.substr(0,wait_time.find('#'));
	
	// do the wait
	WaitForDuration(wait_time);
	
	// advance to the next command in the command list
	++current_command;
}

// :--------:

void MarcusScheduler::SimpleWaitForDuration(std::string wait_string){
	// a simple function to sleep for a given time
	
	// convert wait time string to integer
	int wait_seconds;
	try{
		wait_seconds = std::stoi(wait_string);
	} catch(...){
		// bad parsing.
		Log("MarcusScheduler::SimpleWaitForDuration - Failed to parse wait time of "+wait_string,v_error,verbosity);
		return;
	}
	
	// do the wait
	sleep(wait_seconds);
	
}

// :--------:

void MarcusScheduler::WaitForDuration(std::string wait_string){
	// a wait function with pretty printing to stdout
	printf("Waiting for %s seconds\n",wait_string.c_str());
	
	// parse the wait time
	std::stringstream tmp(wait_string);
	long wait;
	tmp>>wait;
	
	// convert times to boost. we'll have some extra counters as we'll print out
	// how much longer we have in the wait, updating once a second.
	m_period=boost::posix_time::seconds(wait);
	boost::posix_time::ptime current(boost::posix_time::second_clock::local_time());
	boost::posix_time::ptime last(boost::posix_time::second_clock::local_time());
	boost::posix_time::time_duration lapse(m_period - (current - last));
	
	// form a printout string. Rather than using a new line for each update,
	// which can flood the screen with a big countdown, we'll instead overwrite
	// the same section of the terminal, by using backspaces before each new write.
	// This requires using a fixed length buffer, since the number of printed
	// characters in the remaining time may vary as the counter progresses
	char lapse_buffer[100];
	// determine how many characters out number needs to be, and form a suitable
	// printf format string. we'll pad with 0's.
	int string_length = wait_string.length();
	std::string string_length_string = std::to_string(string_length);
	std::string lapse_buffer_format = "%0"+string_length_string+"ld";
	
	// print the first line and flush to stdout
	printf(lapse_buffer_format.c_str(),lapse.total_seconds());
	fflush(stdout);
	
	// now the countdown. Loop while remaining time is positive.
	while(!lapse.is_negative()){
		// format the remaining time as a fixed width string
		long remaining_secs = lapse.total_seconds();
		string_length = snprintf(lapse_buffer, 100, lapse_buffer_format.c_str(), remaining_secs);
		
		// erase the current lapsed time
		for(int i=0; i<string_length; ++i) printf("\b");
		
		// overwrite it with the new lapsed time
		printf(lapse_buffer_format.c_str(),remaining_secs);
		fflush(stdout);
		
		// sleep for 1 second
		sleep(1);
		
		// update our remaining time
		current=boost::posix_time::second_clock::local_time();
		lapse=boost::posix_time::time_duration(m_period - (current - last));
	}
}

// ««-------------- ≪ °◇◆◇° ≫ --------------»»

void MarcusScheduler::DoAnalyse(std::string the_command){
	Log("Analysing measurements",v_debug,verbosity);
	
	// we may only want to make plots for one particular LED, not all of them.
	
	// strip off the LED, given after the 'analyse' prefix
	std::string ledToAnalyse = the_command.substr(8, std::string::npos);
	
	// trim comments & trailing whitespace
	ledToAnalyse = ledToAnalyse.substr(0,ledToAnalyse.find('#'));
	ledToAnalyse = ledToAnalyse.substr(0,ledToAnalyse.find(' '));
	
	// check it's a valid LED name
	if(LED_states.count(ledToAnalyse)==0){
		Log("MarcusScheduler::DoAnalyse - unknown LED name "+ledToAnalyse+" given to Analyse command",v_error,verbosity);
	}
	
	// otherwise qeueue up the analyse action
	else {
		std::string json_string = "{\"Analyse\":\"Analyse\",\"ledToAnalyse\":\"" + ledToAnalyse + "\"}";
		Log(std::string("Queuing action: ")+json_string,v_debug,verbosity);
		m_data->CStore.JsonParser(json_string);
	}
	
	// advance to the next command
	++current_command;
}

// ««-------------- ≪ °◇◆◇° ≫ --------------»»

void MarcusScheduler::DoSave(std::string the_command){
	Log("Saving measurements",v_debug,verbosity);
	
	// get the filename to save it as
	std::string filename = the_command.substr(5,std::string::npos);
	
	// trim comments & trailing whitespace
	filename = filename.substr(0,filename.find('#'));
	filename = filename.substr(0,filename.find(' '));
	
	// if we're looping and not overwriting save files, add the loop index to the output filename
	std::string output_file = filename;
	if(looping && overwrite_saves){
		std::string filenamesub = filename.substr(0,filename.find_last_of('.'));
		output_file = filenamesub+"_"+std::to_string(loop_count)+".root";
	}
	
	// queue up the save action
	std::string json_string = "{\"Save\":\"Save\",\"Filename\":\""+output_file
		+"\",\"Overwrite\",\""+std::to_string(overwrite_saves)+"\"}";
	Log(std::string("Queuing action: ")+json_string,v_debug,verbosity);
	m_data->CStore.JsonParser(json_string);
	
	// advance to the next command
	++current_command;
}

// ««-------------- ≪ °◇◆◇° ≫ --------------»»

void MarcusScheduler::DoValves(std::string the_command){
	// control the inlet/outlet valves
	
	// strip off the preceding 'valve' to get the action
	std::string open_or_close = the_command.substr(6,std::string::npos);
	
	// strip off trailing comments
	open_or_close = open_or_close.substr(0,open_or_close.find(' '));
	
	// check we recognise the action, and form the json string
	Log(std::string("setting valve to: \"")+open_or_close+std::string("\""),v_debug,verbosity);
	std::string json_string;
	if(open_or_close=="open"){
		json_string="{\"Valve\":\"OPEN\"}";
	} else if(open_or_close=="close"){
		json_string="{\"Valve\":\"CLOSE\"}";
	} else {
		Log("MarcusScheduler::DoValves - Unknown valve state: '"+open_or_close+"'",v_error,verbosity);
	}
	
	// queue up the action
	m_data->CStore.JsonParser(json_string);
	
	// advance to the next command
	++current_command;
}

// ««-------------- ≪ °◇◆◇° ≫ --------------»»

void MarcusScheduler::StartLoop(std::string the_command){
	// note the start location of a segment to loop
	// we use vectors to permit nested loops
	loop_starts.push_back(current_command+1);
	
	// add a counter to track iterations of this loop
	loop_counts.push_back(0);
	
	// see if this has a fixed number of loops to perform
	size_t iterations;
	try{
		// strip off the 'start_loop' prefix
		std::string iteration_string = 
		    iteration_string.substr(iteration_string.find_first_not_of(' '),std::string::npos);
		
		// try to convert to an integer loop count
		iterations = std::stoi(iteration_string);
		
	} catch(...){
		// if not, assume infinite loops.
		iterations = 0;
	}
	
	// add the number of iterations this loop should perform
	loop_iterations.push_back(iterations);
	
	// advance to the next command
	++current_command;
}

void MarcusScheduler::EndLoop(std::string the_command){
	// found the end of a segment to loop
	// increment flattened loop counter used for unique output file names
	++loop_count;
	
	// increment the loop counter for this loop
	++loop_counts.back();
	
	// check if we've now performed all requested iterations of this loop
	if(loop_iterations.back()>0 && loop_counts.back()>=loop_counts.back()){
		// loop complete - remove it from our set of loops
		loop_starts.pop_back();
		loop_counts.pop_back();
		loop_iterations.pop_back();
		
		// go to next command
		++current_command;
	} else {
		// otherwise go back to the start of this loop
		current_command=loop_starts.back();
	}
}

// ««-------------- ≪ °◇◆◇° ≫ --------------»»

// here's the big one.
void MarcusScheduler::DoMeasure(std::string the_command){
	// perform a spectrometer measurement.
	// this will actually require several Execute loops, but we bundle them
	// into one command to ensure the entire thing happens together
	Log(std::string("Performing Measurement step ")+std::to_string(command_step),v_debug,verbosity);
	
	// note measurement time - this is needed by the TraceAverage tool
	m_data->measurment_time= boost::posix_time::ptime(boost::posix_time::second_clock::local_time());
	
	// the actions required are queued by setting appropriate variables into the CStore.
	// downstream tools then check the CStore for these variables, and if present,
	// perform the corresponding action.
	
	// We'll form a JSON string to encapsulate all the necessary variables
	std::string json_string="";
	switch (command_step){
		case 0:
			{
			// first thing, ensure all LEDs are off
			// ====================================
			json_string = "{\"R\":\"0\",\"G\":\"0\",\"B\":\"0\",\"White\":\"0\",\"385\":\"0\",\"260\":\"0\",\"275\":\"0\",\"LED\":\"Change\"}";
			++command_step;
			//break;  // XXX
			}
			
		case 1:
			{
			// now take a dark measurement
			// ===========================
			// if we have multiple measurements with a dark each, they need to be named accordingly
			// parse the set of LEDs in this measurement to append to the trace name
			std::string led_list = the_command.substr(8,the_command.find('#')-8);
			while(led_list.back()==' ') led_list.pop_back();              // trim trailing spaces
			std::replace( led_list.begin(), led_list.end(), ' ', '_' );   // replace spaces with underscores
			measurement_name = "Dark_"+led_list;
			json_string = "{\"Measure\":\"Start\",\"Trace\":\""+measurement_name+"\"}";
			++command_step;
			//break;  // XXX
			}
			
		case 2:
			// XXX taking a dark for _every_ measurement may be excessive.
			// Disable the 'break' on case 0 and 1 to let it run straight through to 2.
			// all these do is set json_string and measurement_name, the former is overwritten
			// and the latter is useable.
			{
			// then turn on the desired LEDs
			// =============================
			// these should be listed in order after the 'measure' string
			Log("will parse led list",v_debug,verbosity);
			// initialize LED_states to all off
			LED_states = off_LED_states;
			// obtain the list of LEDs starting after 'measure' and ending at first '#'
			// to string any trailing comments
			std::string led_list = the_command.substr(8,the_command.find('#')-8);
			while(led_list.back()==' ') led_list.pop_back();
			// split up the list of LEDs and enable all those that appear and are known
			
			Log(std::string("parsing LED list: \"")+led_list+"\"",v_debug,verbosity);
			int last_space=0;
			while(true){
				int next_space = led_list.find(' ',last_space);
				std::string next_led_name = led_list.substr(last_space, next_space-last_space);
				Log(std::string("Setting LED ")+next_led_name+" to ON for next measurement",v_debug,verbosity);
				// check this is a valid LED name
				// compatibility
				if(LED_states.count(next_led_name)){
					LED_states.at(next_led_name) = 1; // enable this LED
				} else if(next_led_name!="Dark" && next_led_name!="None"){
					Log("MarcusScheduler::DoMeasure - Unknown LED "+next_led_name,v_error,verbosity);
				}
				if(next_space!=std::string::npos){
					last_space = led_list.find_first_not_of(' ',next_space+1);
				} else {
					break;
				}
			}
			Log("building measurement name",v_debug,verbosity);
			// add the LED states to the JSON string
			json_string="{";
			for(const std::pair<std::string,int>& an_led_state : LED_states){
				json_string += "\""+an_led_state.first+"\":\""+an_led_state.second+"\",";
			}
			// use same measurement name as for Dark, but remove initial 'Dark_'
			measurement_name = measurement_name.substr(5,std::string::npos);
			// add the LED change command
			json_string += "\"LED\":\"Change\"}";
			++command_step;
			break;
			}
			
		case 3:
			{
			// now take the light measurement
			// ==============================
			json_string = "{\"Measure\":\"Start\",\"Trace\":\""+measurement_name+"\"}";
			++command_step;
			break;
			}
			
		case 4:
			{
			// finally, disable the LEDs. VERY IMPORTANT.
			// ==========================================
			json_string = "{\"R\":\"0\",\"G\":\"0\",\"B\":\"0\",\"White\":\"0\",\"385\":\"0\",\"260\":\"0\",\"275\":\"0\",\"LED\":\"Change\"}";
			// time to move to the next command
			command_step=0;
			++current_command;
			break;
			}
			
		default:
			Log("MarcusScheduler::DoMeasure - measurement process step "+std::to_string(command_step)
				+" unknown! How did we get here?!",v_error,verbosity);
		
	}  // end switch over command_step
	
	// queue the action for this step of the measurement process
	Log(std::string("Queuing action: ")+json_string,v_debug,verbosity);
	m_data->CStore.JsonParser(json_string);
}
