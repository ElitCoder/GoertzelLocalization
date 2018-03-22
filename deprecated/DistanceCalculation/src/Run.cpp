#include "Run.h"
#include "Recording.h"
#include "WavReader.h"
#include "Settings.h"

#include <iostream>
#include <cmath>
#include <fstream>
#include <climits>

using namespace std;

static const int FREQ_N = 16;
static const int FREQ_FREQ = 4000;
static const double FREQ_REDUCING = 0.001;
static const double FREQ_THRESHOLD = 0.05;

extern Settings g_settings;

static double calculateDistance(Recording& master, Recording& recording) {
	long long r12 = recording.getTonePlayingWhen(master.getId());
	long long p1 = master.getTonePlayingWhen(master.getId());
	long long r21 = master.getTonePlayingWhen(recording.getId());
	long long p2 = recording.getTonePlayingWhen(recording.getId());
	
	// TODO: remove this, it's unnecessary
	// T12 = Tp + Dt
	// T21 = Tp - Dt
	double T12 = r12 - p1;
	double T21 = r21 - p2;
	
	double Dt = -(static_cast<double>(T21) - static_cast<double>(T12)) / 2;
	
	double Tp1 = T12 - Dt;
	double Tp2 = T21 + Dt;
	
	double Tp = (Tp1 + Tp2) / 2;
	double Tp_sec = Tp / 48000;
	
	return abs(Tp_sec * 343);
}

void writeLocalization(vector<Recording>& recordings) {
	ofstream file("../../Localization/live_localization.txt");
	
	if (!file.is_open()) {
		cout << "Warning: could not open file for writing results\n";
		
		return;
	}
	
	file << to_string(recordings.size()) << endl;
	
	for (auto& recording : recordings)
		file << recording.getIP() << endl;
	
	for (size_t i = 0; i < recordings.size(); i++) {
		Recording& master = recordings.at(i);
		
		for (size_t j = 0; j < recordings.size(); j++) {
			if (i == j)
				file << to_string(0) << endl;
			else
			 	file << to_string(master.getDistance(j)) << endl;
		}
	}
	
	file.close();
}

void writeLocalization3D(vector<Recording>& recordings) {
	ofstream file("../../Localization3D/live_localization.txt");
	
	if (!file.is_open()) {
		cout << "Warning: could not open file for writing results\n";
		
		return;
	}
	
	file << to_string(recordings.size()) << endl;
	
	for (size_t i = 0; i < recordings.size(); i++) {
		Recording& master = recordings.at(i);
		
		file << master.getIP() << endl;
		
		for (size_t j = 0; j < recordings.size(); j++) {
			if (i == j)
				file << to_string(0) << endl;
			else
			 	file << to_string(master.getDistance(j)) << endl;
		}
	}
	
	file.close();
}

static void writeServerResults(vector<Recording>& recordings) {
	ofstream file("server_results.txt");
	
	if (!file.is_open()) {
		cout << "Warning: could not open file for writing server results\n";
		
		return;
	}
	
	for (size_t i = 0; i < recordings.size(); i++) {
		Recording& master = recordings.at(i);
		
		for (size_t j = 0; j < recordings.size(); j++) {
			file << master.getIP() << " " << recordings.at(j).getIP() << " " << to_string(master.getDistance(j)) << endl;
		}
	}
	
	file.close();
}

static vector<Recording> analyzeSound(const vector<string>& filenames, const vector<string>& ips, int type_localization) {
	vector<Recording> recordings;
	
	for (size_t i = 0; i < filenames.size(); i++) {
		string filename = filenames.at(i);
		
		recordings.push_back(Recording(ips.at(i)));
		
		Recording& recording = recordings.back();
		WavReader::read(filename, recording.getData());
		
		switch (type_localization) {
			case RUN_GOERTZEL: recording.findStartingTones(filenames.size(), FREQ_N, FREQ_THRESHOLD, FREQ_REDUCING, FREQ_FREQ);
				break;
				
			case RUN_WHITE_NOISE: recording.findStartingTonesAmplitude(filenames.size());
				break;
		}
	}
	
	return recordings;
}

static void runDistanceCalculation(vector<Recording>& recordings) {
	cout << endl;
	
	for (size_t i = 0; i < recordings.size(); i++) {
		Recording& master = recordings.at(i);
		
		master.addDistance(i, 0);
		
		for (size_t j = 0; j < recordings.size(); j++) {
			if (j == i)
				continue;
			
			Recording& recording = recordings.at(j);
			double distance = calculateDistance(master, recording);
			master.addDistance(j, distance);
				
			//if (j > i) {
				cout << "Distance from " << master.getLastIP() << " -> " << recording.getLastIP() << " is "  << distance << " m\n";	
			//}
		}
	}
	
	writeLocalization(recordings);
	writeLocalization3D(recordings);
	
	if (g_settings.has("-ws"))
		writeServerResults(recordings);
}

namespace Run {
	void runWhiteNoise(const vector<string>& filenames, const vector<string>& ips) {
		cout << "Debug: running white noise localization\n";
		
		auto recordings = analyzeSound(filenames, ips, RUN_WHITE_NOISE);
		runDistanceCalculation(recordings);
	}
	
	void runGoertzel(const vector<string>& filenames, const vector<string>& ips) {
		cout << "Debug: running goertzel localization\n";
		
		auto recordings = analyzeSound(filenames, ips, RUN_GOERTZEL);
		runDistanceCalculation(recordings);
		
		/*
		vector<Recording> recordings;
		
		for (size_t i = 0; i < filenames.size(); i++) {
			string filename = filenames.at(i);
			
			recordings.push_back(Recording(ips.at(i)));
			
			Recording& recording = recordings.back();
			WavReader::read(filename, recording.getData());
			
			recording.findStartingTones(filenames.size(), FREQ_N, FREQ_THRESHOLD, FREQ_REDUCING, FREQ_FREQ);
		}
		*/
		
		/*
		cout << endl;
		
		for (size_t i = 0; i < recordings.size(); i++) {
			Recording& master = recordings.at(i);
			
			master.addDistance(i, 0);
			
			for (size_t j = 0; j < recordings.size(); j++) {
				if (j == i)
					continue;
				
				Recording& recording = recordings.at(j);
				double distance = calculateDistance(master, recording);
				master.addDistance(j, distance);
					
				if (j > i) {
					cout << "Distance from " << master.getLastIP() << " -> " << recording.getLastIP() << " is "  << distance << " m\n";	
				}
			}
		}
		*/
		
		/*
		for (size_t i = 0; i < recordings.size(); i++) {
			auto& recording = recordings.at(i);
			
			for (size_t j = 0; j < recordings.size(); j++) {
				if (i == j)
					continue;
					
				cout << i << " find " << j << " at " << recording.getTonePlayingWhen(j) / (double)48000 << " s\n";
				cout << i << " to " << j << " takes " << to_string(recording.getFrameDistance(j, FIRST) / (double)48000) << " s\n";
				//cout << j << " to " << i << " takes " << recording.getFrameDistance(j, SECOND) / (double)48000 << " s\n";
			}
		}
		
		*/
		
		//cout << endl;	
		
		/*
		auto deltas = calculateDeltas(recordings);
		auto delta_differences = compareDeltaDifferences(deltas);
		auto delta_values = getDeltaValues(deltas, delta_differences);
		*/
		
		//cout << "Delta containers:\n";
		
		//for (size_t i = 0; i < delta_values.size(); i++)
		//	cout << delta_values.at(i) << endl;
		
		/*
		for (int i = 0; i < num_recordings; i++) {
			Recording& master = recordings.at(i);
			
			for (int j = 0; j < num_recordings; j++) {
				if (j == i)
					continue;
				
				Recording& recording = recordings.at(j);
				double mean = (*find(delta_values.begin(), delta_values.end(), DeltaContainer(i, j))).mean();
				double distance = calculateDistance(master, recording, mean);
					
				if (j > i) {
					cout << "Distance from " << master.getLastIP() << " -> " << recording.getLastIP() << " is "  << distance << " m (old: " << master.getDistance(j) << " m)\n";	
				}
			}
		}	*/
		
		/*
		//writeResults(recordings);
		writeLocalization(recordings);
		writeLocalization3D(recordings);
		
		if (g_settings.has("-ws"))
			writeServerResults(recordings);
			*/
	}
}