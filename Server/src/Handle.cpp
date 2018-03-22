#include "Handle.h"
#include "WavReader.h"
#include "Config.h"
#include "Localization3D.h"
#include "Goertzel.h"

#include <libnessh/SSHMaster.h>

#include <sstream>
#include <iostream>
#include <fstream>
#include <cmath>
#include <memory>
#include <algorithm>
#include <climits>

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

static vector<string> createRunLocalizationScripts(const vector<string>& ips, int play_time, int idle_time, int extra_recording, const string& file) {
	vector<string> scripts;
	
	for (size_t i = 0; i < ips.size(); i++) {
		string script =	"systemctl stop audio*\n";
		script +=		"arecord -D audiosource -r 48000 -f S16_LE -c 1 -d ";
		script +=		to_string(idle_time * 2 + ips.size() * (1 + play_time) + extra_recording);
		script +=		" /tmp/cap";
		script +=		ips.at(i);
		script +=		".wav &\n";
		script +=		"sleep ";
		script +=		to_string(idle_time + i * (play_time + 1));
		script +=		"\n";
		script +=		"aplay -D localhw_0 -r 48000 -f S16_LE /tmp/";
		script += 		file;
		script +=		"\n";
		
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

vector<SpeakerPlacement> Handle::handleRunLocalization(const vector<string>& ips, bool skip_script) {
	if (!skip_script) {
		// Create scripts
		auto scripts = createRunLocalizationScripts(ips, Config::get<int>("speaker_play_length"), Config::get<int>("idle_time"), Config::get<int>("extra_recording"), Config::get<string>("goertzel"));
		
		// Transfer test file
		cout << "Debug: transferring files\n";
		if (!sendSSHFile(ips, "data/" + Config::get<string>("goertzel"), "/tmp/"))
			return vector<SpeakerPlacement>();
			
		cout << "Debug: done\n";
		
		// Send scripts
		cout << "Debug: running scripts\n";
		printSSHOutput(runSSHScript(ips, scripts));
		cout << "Debug: done\n";
		
		// Get recordings
		vector<string> from;
		vector<string> to;
		
		for (auto& ip : ips) {
			from.push_back("/tmp/cap" + ip + ".wav");
			to.push_back("results");
		}
		
		cout << "Debug: getting recordings\n";
		if (!getSSHFile(ips, from, to))
			return vector<SpeakerPlacement>();
			
		cout << "Debug: done\n";
	}
	
	auto distances = Goertzel::runGoertzel(ips);
	cout << "Got distances\n";
	
	if (distances.empty())
		return vector<SpeakerPlacement>();
	
	auto placement = Localization3D::run(distances, Config::get<bool>("fast"));
	cout << "Got placement\n";
	
	vector<SpeakerPlacement> speakers;
	
	for (size_t i = 0; i < ips.size(); i++) {
		SpeakerPlacement speaker(ips.at(i));
		auto& master = distances.at(i);
		
		for (size_t j = 0; j < master.second.size(); j++) {
			cout << "Add distance from " << ips.at(i) << " to " << ips.at(j) << " at " << master.second.at(j) << endl;
			
			speaker.addDistance(ips.at(j), master.second.at(j));
		}
		
		speaker.setCoordinates(placement.at(i));
		
		speakers.push_back(speaker);
	}
	
	return speakers;
}

static vector<string> createTestSpeakerdBsScripts(const vector<string>& ips, int play_time, int idle_time, const string& file) {
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
		script +=		"aplay -D localhw_0 -r 48000 -f S16_LE /tmp/";
		script +=		file;
		script +=		"\n";
		//white_noise_2.wav\n";
		
		cout << "Debug: creating script.. " << script << endl;
		scripts.push_back(script);
	}
	
	return scripts;
}

static short getAverage(const vector<short>& data, size_t start, size_t end) {
	unsigned long long sum = 0;
	
	for (size_t i = start; i < end; i++)
		sum += abs(data.at(i));
		
	return sum / (end - start);
}

static vector<string> createTestSpeakerdBsListeningScripts(const vector<string>& listening_ips, int num_playing, int play_time, int idle_time) {
	vector<string> scripts;
	
	for (size_t i = 0; i < listening_ips.size(); i++) {
		string script =	"systemctl stop audio*\n";
		script +=		"arecord -D audiosource -r 48000 -f S16_LE -c 1 -d ";
		script +=		to_string(idle_time * 2 + num_playing * (1 + play_time));
		script +=		" /tmp/cap";
		script +=		listening_ips.at(i);
		script +=		".wav &\n";
		
		scripts.push_back(script);
	}
	
	return scripts;
}

static vector<string> createTestSpeakerdBsPlayingScripts(const vector<string>& playing_ips, int play_time, int idle_time, const string& filename) {
	vector<string> scripts;
	
	for (size_t i = 0; i < playing_ips.size(); i++) {
		string script =	"systemctl stop audio*\n";
		script +=		"sleep ";
		script +=		to_string(idle_time);
		script +=		"\n";
		script +=		"aplay -D localhw_0 -r 48000 -f S16_LE /tmp/";
		script +=		filename;
		script +=		"\n";
		
		scripts.push_back(script);
	}
	
	return scripts;
}

SpeakerdBs testSpeakerdBsExternal(const vector<string>& playing_ips, const vector<string>& listening_ips, int play_time, int idle_time) {
	auto listening_scripts = createTestSpeakerdBsListeningScripts(listening_ips, playing_ips.size(), play_time, idle_time);
	auto playing_scripts = createTestSpeakerdBsPlayingScripts(playing_ips, play_time, idle_time, Config::get<string>("white_noise"));
	
	// Combine all scripts into one SSHMaster call
	vector<string> all_ips;
	all_ips.insert(all_ips.end(), playing_ips.begin(), playing_ips.end());
	all_ips.insert(all_ips.end(), listening_ips.begin(), listening_ips.end());
	
	vector<string> all_commands;
	all_commands.insert(all_commands.end(), playing_scripts.begin(), playing_scripts.end());
	all_commands.insert(all_commands.end(), listening_scripts.begin(), listening_scripts.end());
	
	// Send test tone to playing IPs
	if (!sendSSHFile(playing_ips, "data/" + Config::get<string>("white_noise"), "/tmp/"))
		return SpeakerdBs();
		
	return SpeakerdBs();	
}

SpeakerdBs Handle::handleTestSpeakerdBs(const vector<string>& ips, int play_time, int idle_time, int num_external, bool skip_script) {
	if (num_external > 0) {
		cout << "Warning: external mics are not supported right now\n";
		
		return SpeakerdBs();
	}
	
	if (!skip_script) {
		auto scripts = createTestSpeakerdBsScripts(ips, play_time, idle_time, Config::get<string>("white_noise"));
		
		// Transfer test file
		cout << "Debug: transferring files\n";
		if (!sendSSHFile(ips, "data/" + Config::get<string>("white_noise"), "/tmp/"))
			return SpeakerdBs();
			
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
		if (!getSSHFile(ips, from, to))
			return SpeakerdBs();
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
		
		if (data.empty())
			return SpeakerdBs();
		
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