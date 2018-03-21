#include "Handle.h"

#include <libnessh/SSHMaster.h>

#include <sstream>
#include <iostream>
#include <fstream>
#include <cmath>
#include <memory>
#include <algorithm>

using namespace std;

enum {
	RUN_LOCALIZATION_GOERTZEL,
	RUN_LOCALIZATION_WHITE_NOISE
};

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
	string command = "amixer -c1 sget 'Headphone' && amixer -c1 sget 'Capture' && amixer -c1 sget 'PGA Boost'; wait";
	vector<string> commands(ips.size(), command);
	
	return runSSHScript(ips, commands);
}

// TODO: this should be done async later on (if necessary)
SSHOutput Handle::handleSetSpeakerVolumeAndCapture(const vector<string>& ips, const vector<double>& volumes, const vector<double>& captures, const vector<int>& boosts) {
	vector<string> commands;
	
	for (size_t i = 0; i < volumes.size(); i++) {
		string volume = to_string(static_cast<int>(round(volumes.at(i))));
		string capture = to_string(static_cast<int>(round(captures.at(i))));
		string boost = to_string(boosts.at(i));
		
		string command = "amixer -c1 sset 'Headphone' " + volume + " && ";
		command += "amixer -c1 sset 'Capture' " + capture + " && ";
		command += "amixer -c1 sset 'PGA Boost' " + boost + "; wait";
		
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
// * Introduce some kind of path handler and maybe run this script from Server, in other words move all functionality to Server instead of these modules 
vector<SpeakerPlacement> Handle::handleRunLocalization(const vector<string>& ips, int type_localization) {
	writeDistanceSpeakerIps(ips);
	
	vector<SpeakerPlacement> speakers;
	
	if (!system(NULL)) {
		cout << "Warning: shell is not available\n";
		
		return speakers;
	}
	
	switch (type_localization) {
		case RUN_LOCALIZATION_GOERTZEL: system("./run_localization.sh; wait");
			break;
			
		case RUN_LOCALIZATION_WHITE_NOISE: system("./run_localization_white_noise.sh; wait");
			break;
			
		default: cout << "Warning: client trying to run unknown speaker localization type: " << type_localization << endl;
			return speakers;
	}
	
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