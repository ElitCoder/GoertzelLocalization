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
#include <cassert>
#include <thread>

using namespace std;

static SSHOutput runSSHScript(const vector<string>& ips, const vector<string>& commands) {
	if (ips.size() != commands.size()) {
		cout << "Error: ips.size() != commands.size()\n";
		
		return SSHOutput();
	}
	
	SSHMaster master;
	
	cout << "SSH: connecting to clients.. " << flush;
	if (!master.connect(ips, "pass"))
		return SSHOutput();
	cout << "done\n";
	
	for (size_t i = 0; i < ips.size(); i++) {
		cout << "SSH: running command (" << ips.at(i) << ")\n**************\n";
		cout << commands.at(i) << "**************\n\n";
	}
	
	cout << "SSH: running commands.. " << flush;	
	master.setSetting(SETTING_ENABLE_SSH_OUTPUT_VECTOR_STYLE, true);
	auto outputs = master.command(ips, commands);
	master.setSetting(SETTING_ENABLE_SSH_OUTPUT_VECTOR_STYLE, false);
	cout << "done\n";
	
	return outputs;	
}

static bool sendSSHFile(const vector<string>& ips, string from, string to) {
	SSHMaster master;
	
	cout << "SSH: connecting to clients.. " << flush;
	if (!master.connect(ips, "pass"))
		return false;
	cout << "done\n";
		
	vector<string> froms(ips.size(), from);
	vector<string> tos(ips.size(), to);
		
	cout << "SSH: transferring to remote.. " << flush;
	auto status = master.transferRemote(ips, froms, tos);
	cout << "done\n";
	
	return status;
}

static bool getSSHFile(const vector<string>& ips, vector<string>& from, vector<string>& to) {
	SSHMaster master;
	
	cout << "SSH: connecting to clients.. " << flush;
	if (!master.connect(ips, "pass"))
		return false;
	cout << "done\n";
		
	cout << "SSH: transferring to local.. " << flush;
	auto status = master.transferLocal(ips, from, to, true);
	cout << "done\n";
	
	return status;
}

// TODO: this should be done async later on (if necessary)
bool Handle::handleSetSpeakerVolumeAndCapture(const vector<string>& ips, const vector<int>& volumes, const vector<int>& captures, const vector<int>& boosts) {
	vector<string> commands;
	
	for (size_t i = 0; i < volumes.size(); i++) {
		string volume = to_string(volumes.at(i));
		string capture = to_string(captures.at(i));
		string boost = to_string(boosts.at(i));
		
		string command = "amixer -c1 sset 'Headphone' " + volume + "; wait; ";
		command += "amixer -c1 sset 'Capture' " + capture + "; wait; ";
		command += "dspd -s -m; wait; dspd -s -u limiter; wait; dspd -s -u static; wait; ";
		command += "amixer -c1 sset 'PGA Boost' " + boost + "; wait\n";
		
		commands.push_back(command);
	}
	
	return !runSSHScript(ips, commands).empty();
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
		script +=		"proc1=$!\n";
		script +=		"sleep ";
		script +=		to_string(idle_time + i * (play_time + 1));
		script +=		"\n";
		script +=		"aplay -D localhw_0 -r 48000 -f S16_LE /tmp/";
		script += 		file;
		script +=		"\n";
		script +=		"wait $proc1\n";
		script +=		"systemctl start audio-conf; wait\n";
		
		scripts.push_back(script);
	}
	
	return scripts;
}

void printSSHOutput(SSHOutput outputs) {
	for (auto& output : outputs)
		for (auto& line : output.second)
			cout << "SSH (" << output.first << "): " << line << endl;
}

vector<SpeakerPlacement> Handle::handleRunLocalization(const vector<string>& ips, bool skip_script) {
	if (!skip_script) {
		// Create scripts
		int play_time = Config::get<int>("speaker_play_length");
		int idle_time = Config::get<int>("idle_time");
		int extra_recording = Config::get<int>("extra_recording");
		
		auto scripts = createRunLocalizationScripts(ips, play_time, idle_time, extra_recording, Config::get<string>("goertzel"));
		
		// Transfer test file
		if (!sendSSHFile(ips, "data/" + Config::get<string>("goertzel"), "/tmp/"))
			return vector<SpeakerPlacement>();
		
		// Send scripts
		printSSHOutput(runSSHScript(ips, scripts));
		
		// Get recordings
		vector<string> from;
		vector<string> to;
		
		for (auto& ip : ips) {
			from.push_back("/tmp/cap" + ip + ".wav");
			to.push_back("results");
		}
		
		if (!getSSHFile(ips, from, to))
			return vector<SpeakerPlacement>();
	}
	
	auto distances = Goertzel::runGoertzel(ips);
	
	if (distances.empty())
		return vector<SpeakerPlacement>();
	
	auto placement = Localization3D::run(distances, Config::get<bool>("fast"));
	
	vector<SpeakerPlacement> speakers;
	
	for (size_t i = 0; i < ips.size(); i++) {
		SpeakerPlacement speaker(ips.at(i));
		auto& master = distances.at(i);
		
		for (size_t j = 0; j < master.second.size(); j++) {
		//	cout << "Add distance from " << ips.at(i) << " to " << ips.at(j) << " at " << master.second.at(j) << endl;
			
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
		script +=		to_string(idle_time * 3 + ips.size() * (1 + play_time));
		script +=		" /tmp/cap";
		script +=		ips.at(i);
		script +=		".wav &\n";
		script +=		"proc1=$1\n";
		script +=		"sleep ";
		script +=		to_string(idle_time + i * (play_time + 1));
		script +=		"\n";
		script +=		"aplay -D localhw_0 -r 48000 -f S16_LE /tmp/";
		script +=		file;
		script +=		"\n";
		script +=		"wait $proc1\n";
		script +=		"systemctl start audio-conf; wait\n";
		
		scripts.push_back(script);
	}
	
	return scripts;
}

static short getRMS(const vector<short>& data, size_t start, size_t end) {
	unsigned long long sum = 0;
	
	for (size_t i = start; i < end; i++)
		sum += (data.at(i) * data.at(i));
		
	sum /= (end - start);
	
	return sqrt(sum);
}

static vector<string> createTestSpeakerdBsListeningScripts(const vector<string>& listening_ips, int num_playing, int play_time, int idle_time) {
	vector<string> scripts;
	
	for (size_t i = 0; i < listening_ips.size(); i++) {
		string script =	"systemctl stop audio*\n";
		script +=		"arecord -D audiosource -r 48000 -f S16_LE -c 1 -d ";
		script +=		to_string(idle_time * 3 + num_playing * (1 + play_time));
		script +=		" /tmp/cap";
		script +=		listening_ips.at(i);
		script +=		".wav\n";
		script +=		"systemctl start audio-conf; wait\n";
		
		scripts.push_back(script);
	}
	
	return scripts;
}

static vector<string> createTestSpeakerdBsPlayingScripts(const vector<string>& playing_ips, int play_time, int idle_time, const string& filename) {
	vector<string> scripts;
	
	for (size_t i = 0; i < playing_ips.size(); i++) {
		string script =	"systemctl stop audio*\n";
		script +=		"sleep ";
		script +=		to_string(idle_time + i * (play_time + 1));
		script +=		"\n";
		script +=		"aplay -D localhw_0 -r 48000 -f S16_LE /tmp/";
		script +=		filename;
		script +=		"\n";
		script +=		"systemctl start audio-conf; wait\n";
		
		scripts.push_back(script);
	}
	
	return scripts;
}

static void testSpeakerdBsExternal(const vector<string>& playing_ips, const vector<string>& listening_ips, int play_time, int idle_time) {
	cout << "Playing IPs:\n";
	for_each(playing_ips.begin(), playing_ips.end(), [] (const string& ip) { cout << ip << endl; });
	cout << "Listening IPs:\n";
	for_each(listening_ips.begin(), listening_ips.end(), [] (const string& ip) { cout << ip << endl; });
	
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
		return;
		
	printSSHOutput(runSSHScript(all_ips, all_commands));
	
	// Get resulting files
	vector<string> from;
	vector<string> to;
	
	for (auto& ip : listening_ips) {
		from.push_back("/tmp/cap" + ip + ".wav");
		to.push_back("results");
	}
	
	if (!getSSHFile(listening_ips, from, to))
		return;
}

SpeakerdBs Handle::handleTestSpeakerdBs(const vector<string>& speakers, const vector<string>& mics, int play_time, int idle_time) {
	vector<string> all_ips(speakers);
	all_ips.insert(all_ips.end(), mics.begin(), mics.end());
	
	return handleTestSpeakerdBs(all_ips, play_time, idle_time, mics.size(), false);
}

SpeakerdBs Handle::handleTestSpeakerdBs(const vector<string>& ips, int play_time, int idle_time, int num_external, bool skip_script, bool do_normalize) {
	if (do_normalize)
		cout << "Debug: normalizing output\n";
		
	vector<string> playing_ips = vector<string>(ips.begin(), ips.begin() + (ips.size() - num_external));
	vector<string> listening_ips = vector<string>(ips.begin() + (ips.size() - num_external), ips.end());
	
	if (!skip_script) {
		if (num_external > 0) {
			testSpeakerdBsExternal(playing_ips, listening_ips, play_time, idle_time);
		} else {
			auto scripts = createTestSpeakerdBsScripts(ips, play_time, idle_time, Config::get<string>("white_noise"));
			
			// Transfer test file
			if (!sendSSHFile(ips, "data/" + Config::get<string>("white_noise"), "/tmp/"))
				return SpeakerdBs();
			
			// Send scripts
			printSSHOutput(runSSHScript(ips, scripts));
			
			// Get resulting files
			vector<string> from;
			vector<string> to;
			
			for (auto& ip : ips) {
				from.push_back("/tmp/cap" + ip + ".wav");
				to.push_back("results");
			}
			
			if (!getSSHFile(ips, from, to))
				return SpeakerdBs();
		}
	}
	
	// Make sure only the listening IPs are used in the calculation
	if (num_external == 0)
		listening_ips = playing_ips;
	
	// TODO: maybe noise levels are more accurate to use since they represent how loud the mic is supposedly recording
	//		 instead of normalizing on own speaker volume which is recorded
	// Analyze files
	SpeakerdBs results;
	size_t normalized_noise = 0;
	
	for (size_t i = 0; i < listening_ips.size(); i++) {
		string filename = "results/cap" + listening_ips.at(i) + ".wav";
		
		vector<short> data;
		WavReader::read(filename, data);
		
		if (data.empty())
			return SpeakerdBs();

		size_t noise_start = idle_time + playing_ips.size() * (play_time + 1);
		noise_start *= 48000;
		size_t noise_level = getRMS(data, noise_start, data.size());
		
		if (normalized_noise == 0)
			normalized_noise = noise_level;
		
		cout << "Debug: noise_level " << noise_level << endl;
		cout << "Debug: normalized_noise " << normalized_noise << endl;
			
		double normalize = (double)normalized_noise / noise_level;
		
		cout << "Debug: normalize " << normalize << endl;
			
		vector<pair<string, double>> decibels;		
			
		for (size_t j = 0; j < playing_ips.size(); j++) {
			size_t record_at = static_cast<double>((idle_time + j * (play_time + 1) + 0.3) * 48000);
			size_t average = getRMS(data, record_at, record_at + (48000 / 2));
			size_t average_normalized = average;
			
			if (do_normalize)
				average_normalized *= normalize;
			
			cout << "Debug: sound_average " << average << endl;
			cout << "Debug: sound_average_normalized " << average_normalized << endl;
			
			double db = 20 * log10(average_normalized / (double)SHRT_MAX);
			
			cout << "IP " << listening_ips.at(i) << " hears " << playing_ips.at(j) << " at " << db << " dB\n";
			
			decibels.push_back({ playing_ips.at(j), db });
		}
		
		results.push_back({ listening_ips.at(i), decibels });
	}
	
	return results;
}

vector<bool> Handle::checkSpeakerOnline(const vector<string>& ips) {
	SSHMaster master;
	
	return master.connectResult(ips, "pass");
}

static vector<string> createSoundImageScripts(const vector<string>& speakers, const vector<string>& mics, int play_time, int idle_time, const string& filename) {
	vector<string> scripts;
	
	for (size_t i = 0; i < speakers.size(); i++) {
		string script = "systemctl stop audio*\n";
		script +=		"sleep " + to_string(idle_time) + "\n";
		script +=		"aplay -D localhw_0 -r 48000 -f S16_LE /tmp/" + filename + "\n";
		script +=		"systemctl start audio-conf; wait\n";
		
		scripts.push_back(script);
	}
	
	for (size_t i = 0; i < mics.size(); i++) {
		string script = "systemctl stop audio*\n";
		script +=		"arecord -D audiosource -r 48000 -f S16_LE -c 1 -d " + to_string(idle_time * 4 + play_time);
		script +=		" /tmp/cap";
		script +=		mics.at(i);
		script +=		".wav\n";
		script +=		"systemctl start audio-conf; wait\n";
		
		scripts.push_back(script);
	}
	
	return scripts;
}

vector<pair<string, double>> Handle::handleSoundImage(const vector<string>& speakers, const vector<string>& mics, int play_time, int idle_time) {
	// Not needed?
	// Get speaker dB in order to normalize sound level
	//auto speaker_dbs = handleTestSpeakerdBs(speakers, mics, play_time, idle_time);
	
	// Get sound image from available microphones
	// TODO: Include external microphone placements
	auto scripts = createSoundImageScripts(speakers, mics, play_time, idle_time, Config::get<string>("white_noise"));
	
	vector<string> all_ips(speakers);
	all_ips.insert(all_ips.end(), mics.begin(), mics.end());
	
	if (!sendSSHFile(speakers, "data/" + Config::get<string>("white_noise"), "/tmp/"))
		return vector<pair<string, double>>();
	
	// Send scripts
	printSSHOutput(runSSHScript(all_ips, scripts));
	
	// Get resulting files
	vector<string> from;
	vector<string> to;
	
	for (auto& ip : mics) {
		from.push_back("/tmp/cap" + ip + ".wav");
		to.push_back("results");
	}
	
	if (!getSSHFile(mics, from, to))
		return vector<pair<string, double>>();
		
	vector<pair<string, double>> final_result;
	size_t normalized_noise = 0;
		
	for (size_t i = 0; i < mics.size(); i++) {
		string filename = "results/cap" + mics.at(i) + ".wav";
		
		vector<short> data;
		WavReader::read(filename, data);
		
		if (data.empty())
			return vector<pair<string, double>>();
			
		size_t noise_start = idle_time + play_time * 2;
		noise_start *= 48000;
		size_t noise_level = getRMS(data, noise_start, data.size());
		
		if (normalized_noise == 0)
			normalized_noise = noise_level;
			
		cout << "Debug: noise_level " << noise_level << endl;
		cout << "Debug: normalized_noise " << normalized_noise << endl;
			
		double normalize = (double)normalized_noise / noise_level;
		
		cout << "Debug: normalize " << normalize << endl;
		
		size_t sound_start = ((double)idle_time + 0.3) * 48000;
		size_t sound_average = getRMS(data, sound_start, sound_start + (48000 / 2));
		size_t sound_average_normalized = sound_average * normalize;
		
		cout << "Debug: sound_average " << sound_average << endl;
		cout << "Debug: sound_average_normalized " << sound_average_normalized << endl;
		
		double db = 20 * log10(sound_average_normalized / (double)SHRT_MAX);
		
		cout << "Debug: db " << db << endl;
		
		final_result.push_back({ mics.at(i), db });
	}	
		
	return final_result;
}