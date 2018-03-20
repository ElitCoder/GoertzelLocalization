#include "Handle.h"

#include <libnessh/SSHMaster.h>

#include <sstream>
#include <iostream>
#include <fstream>
#include <cmath>
#include <memory>
#include <algorithm>

using namespace std;

static SSHOutput runSSHScript(const vector<string>& ips, const vector<string>& commands) {
	SSHMaster master;
	
	if (!master.connect(ips, "pass"))
		return SSHOutput();
		
	master.setSetting(SETTING_ENABLE_SSH_OUTPUT_VECTOR_STYLE, true);
	auto outputs = master.command(ips, commands);
	master.setSetting(SETTING_ENABLE_SSH_OUTPUT_VECTOR_STYLE, false);
	
	return outputs;	
}

SSHOutput Handle::handleGetSpeakerVolumeAndCapture(const vector<string>& ips) {
	string command = "amixer -c1 sget 'Headphone' && amixer -c1 sget 'Capture'; wait";
	vector<string> commands(ips.size(), command);
	
	return runSSHScript(ips, commands);
}

// TODO: this should be done async later on (if necessary)
SSHOutput Handle::handleSetSpeakerVolumeAndCapture(const vector<string>& ips, const vector<double>& volumes, const vector<double>& captures) {
	vector<string> commands;
	
	for (size_t i = 0; i < volumes.size(); i++) {
		string volume = to_string(static_cast<int>(round(volumes.at(i))));
		string capture = to_string(static_cast<int>(round(captures.at(i))));
		
		string command = "amixer -c1 sset 'Headphone' " + volume + " && ";
		command += "amixer -c1 sset 'Capture' " + capture + "; wait";
		
		cout << "Debug: sending command " << command << " to " << ips.at(i) << endl;
		
		commands.push_back(command);
	}
	
	return runSSHScript(ips, commands);
}

static void writeDistanceSpeakerIps(const vector<string>& ips) {
	string filename = "speaker_ips";
	ofstream file(filename);
	
	if (!file.is_open()) {
		cout << "Warning: can't write speaker_ips, the distance calculation will report wrong results\n";
		
		return;
	}
	
	for (auto& ip : ips)
		file << ip << endl;
		
	file.close();
}

// TODO: system() calls are bad and should be replaced, writing to relative path is horrible as well
vector<SpeakerPlacement> Handle::handleRunLocalization(const vector<string>& ips) {
	writeDistanceSpeakerIps(ips);
	
	/*
	// TODO: remove this, make sure everything is built
	executeLocalCommand("cd ../ && ./create.sh && cd Server/; wait");
	executeLocalCommand("mv speaker_ips ../DistanceCalculation/bin/data/");
	
	string distances = executeLocalCommand("cd ../DistanceCalculation/bin/ && ./DistanceCalculation -p 4 -er 0 -f data/speaker_ips -tf testTone.wav -t GOERTZEL -ws TRUE && cd ../../Server/; wait");
	string placements = executeLocalCommand("cd ../Localization/ && ./Localization < live_localization.txt && cd ../Server/; wait");
	
	executeLocalCommand("mv ../DistanceCalculation/bin/server_results.txt results/; wait");
	executeLocalCommand("mv ../Localization/server_placements_results.txt results/; wait");
	*/
	
	vector<SpeakerPlacement> speakers;
	
	if (!system(NULL)) {
		cout << "Warning: shell is not available\n";
		
		return speakers;
	}
	
	system("./run_localization.sh; wait");
	
	ifstream file("results/server_results.txt");
	
	if (!file.is_open()) {
		cout << "Warning: could not open server_results.txt\n";
		
		return speakers;
	}
	
	int num_speakers;
	file >> num_speakers;
	
	for (int i = 0; i < num_speakers; i++) {
		for (int j = 0; j < num_speakers; j++) {
			string from;
			file >> from;
			
			string to;
			file >> to;
			
			double distance;
			file >> distance;
			
			auto iterator = find_if(speakers.begin(), speakers.end(), [&from] (SpeakerPlacement& speaker) { return speaker.getIp() == from; });
			
			if (iterator == speakers.end()) {
				SpeakerPlacement speaker(from);
				speaker.addDistance(to, distance);
				
				speakers.push_back(speaker);
			} else {
				SpeakerPlacement& speaker = (*iterator);
				speaker.addDistance(to, distance);
			}
		}
		
		/*
		SpeakerPlacement speaker(from);
		speaker.setCoordinates({ 0, 0 });
		speaker.addDistance(to, distance);
		
		cout << "Debug: adding speaker " << from << endl;
		
		speakers.push_back(speaker);
		*/
	}
	
	file.close();
	
	ifstream file_placements("results/server_placements_results.txt");
	
	if (!file_placements.is_open()) {
		cout << "Warning: could not open server_placements_results.txt\n";
		
		return speakers;
	}
	
	file_placements >> num_speakers;
	
	for (int i = 0; i < num_speakers; i++) {
		string ip;
		file_placements >> ip;
		
		double x;
		file_placements >> x;
		
		double y;
		file_placements >> y;
		
		cout << "Debug: trying to find IP " << ip << endl;
		
		auto iterator = find_if(speakers.begin(), speakers.end(), [&ip] (SpeakerPlacement& speaker) { return speaker.getIp() == ip; });
		
		if (iterator == speakers.end())
			continue;
			
		(*iterator).setCoordinates({ x, y });	
		
		cout << "Debug: setting coordinates (" << x << ", " << y << ") for " << ip << endl;
	}
	
	file_placements.close();
	
	return speakers;
}