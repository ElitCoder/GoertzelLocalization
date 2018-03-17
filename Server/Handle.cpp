#include "Handle.h"

#include <libnessh/SSHMaster.h>

#include <sstream>
#include <iostream>
#include <fstream>

using namespace std;

static vector<string> getTokens(string input, char delimiter) {
	istringstream stream(input);
	vector<string> tokens;
	string token;
	
	while (getline(stream, token, delimiter))
		if (!token.empty())
			tokens.push_back(token);
	
	return tokens;
}

static vector<vector<double>> readSpeakerSettingsFromFile(vector<string>& ips) {
	vector<vector<double>> returning_values;
	
	for (auto& ip : ips) {
		vector<double> values;
		string tmp;
		
		ifstream file("speaker_results/speakersettings" + ip);
		
		if (!file.is_open()) {
			cout << "Warning: could not open file " << "speaker_results/speakersettings" << ip << endl;
			
			break;
		}
		
		while (getline(file, tmp)) {
			if (tmp.find("Left:") == string::npos) {
				cout << "Warning: could not find specified value\n";
				
				values.push_back(0.0);
				continue;
			}
				
			string filtered = tmp.substr(tmp.find("Left:"));
			
			values.push_back(stod(getTokens(filtered, ' ').at(2)));
		}
			
		file.close();
			
		returning_values.push_back(values);	
	}
	
	return returning_values;
}

vector<vector<double>> Handle::handleGetSpeakerVolumeAndCapture(vector<string>& ips) {
	// SSH to speakers
	// Send script to speakers
	// Get resulting speakersettings
	// Put values into return vector
	
	if (!system(NULL)) {
		cout << "Warning: system() calls are unavailable, no handles will work\n";
		
		return {};
	}
	
	if (!system("mkdir speaker_results && wait"))
		cout << "Warning: could not create folder speaker_results, already existing?\n";
	
	SSHMaster ssh_master;
	
	if (!ssh_master.connect(ips, "pass"))
		return {};

	vector<string> remote_from;
	vector<string> remote_to;
	
	for (size_t i = 0; i < ips.size(); i++) {
		remote_from.push_back("speaker_scripts/getsettings.sh");
		remote_to.push_back("/tmp/");
	}
		
	if (!ssh_master.transferRemote(ips, remote_from, remote_to))
		return {};
		
	vector<string> commands;
	
	for (size_t i = 0; i < ips.size(); i++)
		commands.push_back("cd /tmp/ && chmod +x getsettings.sh && ./getsettings.sh && wait");
		
	if (!ssh_master.command(ips, commands))
		return {};
		
	vector<string> local_from;
	vector<string> local_to;
	
	for (auto& ip : ips) {
		local_from.push_back("/tmp/speakersettings");
		local_to.push_back("./speaker_results/speakersettings" + ip);
	}
	
	ssh_master.setSetting(SETTING_USE_ACTUAL_FILENAME, true);
	
	if (!ssh_master.transferLocal(ips, local_from, local_to, true))
		return {};
		
	ssh_master.setSetting(SETTING_USE_ACTUAL_FILENAME, false);	
		
	return readSpeakerSettingsFromFile(ips);	
		
	/*
	vector<vector<double>> returning_values;
	
	for (auto& ip : ips) {
		vector<double> values;
		string tmp;
		
		ifstream file("speaker_results/speakersettings" + ip);
		
		if (!file.is_open()) {
			cout << "Warning: could not open file " << "speaker_results/speakersettings" << ip << endl;
			
			break;
		}
		
		while (getline(file, tmp))
			values.push_back(stod(getTokens(tmp, ' ').back()));
			
		file.close();
			
		returning_values.push_back(values);	
	}
	
	return returning_values;
	*/
}