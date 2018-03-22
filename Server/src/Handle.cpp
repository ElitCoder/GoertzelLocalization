#include "Handle.h"
#include "WavReader.h"

#include <libnessh/SSHMaster.h>

#include <sstream>
#include <iostream>
#include <fstream>
#include <cmath>
#include <memory>
#include <algorithm>
#include <climits>

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

static bool sendSSHFile(const vector<string>& ips, string from, string to) {
	SSHMaster master;
	
	if (!master.connect(ips, "pass"))
		return false;
		
	vector<string> froms(ips.size(), from);
	vector<string> tos(ips.size(), to);
		
	if (!master.transferRemote(ips, froms, tos))
		return false;
		
	return true;
}

static bool getSSHFile(const vector<string>& ips, vector<string>& from, vector<string>& to) {
	SSHMaster master;
	
	if (!master.connect(ips, "pass"))
		return false;
		
	if (!master.transferLocal(ips, from, to, true))
		return false;
		
	return true;	
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

static vector<string> splitString(const string& input, char delimiter) {
	istringstream stream(input);
	vector<string> tokens;
	string token;
	
	while (getline(stream, token, delimiter)) {
		if (!token.empty())
			tokens.push_back(token);
	}
	
	return tokens;
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
		case RUN_LOCALIZATION_GOERTZEL: system("./data/run_localization.sh; wait");
			break;
			
		case RUN_LOCALIZATION_WHITE_NOISE: system("./data/run_localization_white_noise.sh; wait");
			break;
			
		default: cout << "Warning: client trying to run unknown speaker localization type: " << type_localization << endl;
			return speakers;
	}
	
	ifstream file("results/server_results.txt");
	
	if (!file.is_open()) {
		cout << "Warning: could not open server_results.txt\n";
		
		return speakers;
	}
	
	string line;
	
	while (getline(file, line)) {
		auto tokens = splitString(line, ' ');
		
		string from = tokens.front();
		string to = tokens.at(1);
		double distance = stod(tokens.back());
		
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
	
	file.close();
	
	ifstream file_placements("results/server_placements_results.txt");
	
	if (!file_placements.is_open()) {
		cout << "Warning: could not open server_placements_results.txt\n";
		
		return speakers;
	}
	
	while (getline(file_placements, line)) {
		auto tokens = splitString(line, ' ');
		
		string ip = tokens.front();
		double x = stod(tokens.at(1));
		double y = stod(tokens.at(2));
		double z = stod(tokens.at(3));
		
		cout << "Debug: trying to find IP " << ip << endl;
		
		auto iterator = find_if(speakers.begin(), speakers.end(), [&ip] (SpeakerPlacement& speaker) { return speaker.getIp() == ip; });
		
		if (iterator == speakers.end())
			continue;
			
		(*iterator).setCoordinates({ x, y, z });	
		
		cout << "Debug: setting coordinates (" << x << ", " << y << ", " << z << ") for " << ip << endl;
	}
	
	file_placements.close();
	
	return speakers;
}

static vector<string> createTestSpeakerdBsScripts(const vector<string>& ips, int play_time, int idle_time) {
	vector<string> scripts;
	
	for (size_t i = 0; i < ips.size(); i++) {
		string script =	"systemctl stop audio*\n";
		script +=		"arecord -D audiosource -r 48000 -f S16_LE -c 1 -d ";
		script +=		to_string(idle_time * 2 + ips.size() * (1 + play_time));
		script +=		" /tmp/cap";
		script +=		ips.at(i);
		script +=		".wav &\n";
		script +=		"sleep ";
		script +=		to_string(idle_time + i * (play_time + 1));
		script +=		"\n";
		script +=		"aplay -D localhw_0 -r 48000 -f S16_LE /tmp/white_noise_2.wav\n";
		
		cout << "Debug: creating script.. " << script << endl;
		scripts.push_back(script);
	}
	
	return scripts;
}

void printSSHOutput(SSHOutput outputs) {
	for (auto& output : outputs) {
		for (auto& line : output.second) {
			cout << output.first << ": " << line << endl;
		}
	}
}

static short getAverage(const vector<short>& data, size_t start, size_t end) {
	unsigned long long sum = 0;
	
	for (size_t i = start; i < end; i++)
		sum += abs(data.at(i));
		
	return sum / (end - start);
}

SpeakerdBs Handle::handleTestSpeakerdBs(const vector<string>& ips, int play_time, int idle_time, bool skip_script) {
	if (!skip_script) {
		auto scripts = createTestSpeakerdBsScripts(ips, play_time, idle_time);
		
		// Transfer test file
		cout << "Debug: transferring files\n";
		sendSSHFile(ips, "data/white_noise_2.wav", "/tmp");
		cout << "Debug: done\n";
		
		// Send scripts
		cout << "Debug: running scripts\n";
		printSSHOutput(runSSHScript(ips, scripts));
		cout << "Debug: done\n";
		
		// Get resulting files
		vector<string> from;
		vector<string> to;
		
		for (auto& ip : ips) {
			from.push_back("/tmp/cap" + ip + ".wav");
			to.push_back("results");
		}
		
		cout << "Debug: getting recordings\n";
		getSSHFile(ips, from, to);
		cout << "Debug: done\n";
	}
	
	// TODO: maybe noise levels are more accurate to use since they represent how loud the mic is supposedly recording
	//		 instead of normalizing on own speaker volume which is recorded
	// Analyze files
	SpeakerdBs results;
	size_t nominal_self = 0;
	
	for (size_t i = 0; i < ips.size(); i++) {
		string filename = "results/cap" + ips.at(i) + ".wav";
		
		vector<short> data;
		WavReader::read(filename, data);
		
		// Check own sound level
		size_t own_record_at = static_cast<double>((idle_time + i * (play_time + 1) + 0.3) * 48000);
		size_t own_average = getAverage(data, own_record_at, own_record_at + (48000 / 2));
		
		// Set general nominal level
		if (nominal_self == 0)
			nominal_self = own_average;
			
		double multiplier = (double)nominal_self / own_average;
		
		own_average *= multiplier;
		double own_db = 20 * log10(own_average / (double)SHRT_MAX);
		
		vector<pair<string, double>> decibels;
		decibels.push_back({ ips.at(i), own_db });
			
		for (size_t j = 0; j < ips.size(); j++) {
			if (i == j)
				continue;
				
			// Check the middle 0.5 sec of the sound to get average dB
			size_t record_at = static_cast<double>((idle_time + j * (play_time + 1) + 0.3) * 48000);
			size_t average = getAverage(data, record_at, record_at + (48000 / 2));
			
			// Normalize since all speakers should hear themselves equally
			average *= multiplier;
			double db = 20 * log10(average / (double)SHRT_MAX);
			
			cout << "IP " << ips.at(i) << " hears " << ips.at(j) << " at " << db << " dB\n";
			
			decibels.push_back({ ips.at(j), db });
		}
		
		results.push_back(decibels);
	}
	
	return results;
}