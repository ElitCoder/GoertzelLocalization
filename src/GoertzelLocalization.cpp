#include "WavReader.h"
#include "Goertzel.h"
#include "Recording.h"

#include "matplotlibcpp.h"

#include <iostream>
#include <algorithm>
#include <chrono>
#include <fstream>

#define ERROR(...)	do { fprintf(stderr, "Error: "); fprintf(stderr, __VA_ARGS__); fprintf(stderr, "\n"); exit(1); } while(0)

namespace plt = matplotlibcpp;
using namespace std;

static const int FREQ_N = 16;
static const int FREQ_FREQ = 4000;
static const double FREQ_REDUCING = 0.001;
static const double FREQ_THRESHOLD = 0.01;

static bool RUN_SCRIPTS = true;

int g_playingLength = 2e05;

double calculateDistance(Recording& master, Recording& recording) {
	long long play_1 = 0;
	long long play_2 = 0;
	long long record_1 = 0;
	long long record_2 = 0;
	
	play_1 = master.getTonePlayingWhen(master.getId());
	play_2 = recording.getTonePlayingWhen(recording.getId());
	record_1 = master.getTonePlayingWhen(recording.getId());
	record_2 = recording.getTonePlayingWhen(master.getId());
	
	long long sum = (record_1 + record_2) - (play_1 + play_2);
	
	return abs((static_cast<double>(sum)/2))*343/48000;
}

template<class T>
static void plot(vector<T>& data) {
	vector<int>	tmp;
	for_each(data.begin(), data.end(), [&tmp] (T& element) { tmp.push_back(element); });
	
	plt::plot(tmp);
	plt::show();
}

void printHelp() {
	cout << "Usage: ./GoertzelLocalization <pause length in seconds> <speaker #1 IP> <speaker #2 IP>\n\n# of speakers is arbitrary\n\n1. Creates configuration files for every speaker based on IP, and sends"
		 <<	" the files using scp\n2. Starts the scripts at the same time\n3. Wait until done\n4. Retrieve all recordings from the speakers\n5. Calculate distances using Goertzel\n\n";
}

string createConfig(string& ip, int number, int duration) {
	string config = "";
	config += "systemctl stop audio*\n";
	config += "arecord -Daudiosource -r 48000 -fS16_LE -c1 -d";
	config += to_string(duration);
	config += " /tmp/cap";
	config += ip;
	config += ".wav &\n";
	config += "\n";
	config += "sleep ";
	config += to_string(1 + (g_playingLength / 48000 * (number + 1)));
	config += "\n";
	config += "aplay -Dlocalhw_0 -r 48000 -fS16_LE /tmp/testTone.wav\n\nexit;\n";
	
	return config;
}

vector<string> createFilenames(vector<string>& configs) {
	vector<string> filenames;
	
	for (auto& ip : configs) {
		string filename = "recordings/cap";
		filename += ip;
		filename += ".wav";
		
		filenames.push_back(filename);
		
		cout << "Creating filename: " << filename << endl;
	}
	
	return filenames;
}

vector<string> runSetup(int num_recordings, char** ips) {
	vector<string> configs(ips, ips + num_recordings);
	vector<string> files;
	int duration = g_playingLength /* until first tone */ + num_recordings * g_playingLength + g_playingLength /* to make up for jitter */;
	int duration_seconds = duration / 48000;
	
	cout << num_recordings << " speakers and " << g_playingLength << " pause length, writing " << duration_seconds << " as total duration\n";
	
	if (!system(NULL))
		ERROR("system calls are not available");
		
	system("mkdir scripts");
	system("mkdir recordings");
	system("wait");
	
	for (size_t i = 0; i < configs.size(); i++) {
		string& ip = configs.at(i);
		
		string filename = "scripts/script";
		filename += ip;
		filename += ".sh";
		
		files.push_back(filename);
		
		ofstream file(filename);
		
		if (!file.is_open()) {
			cout << "Warning: could not open file " << filename << endl;
			continue;
		}
		
		file << createConfig(ip, i, duration_seconds);
		
		file.close();
		
		cout << "Created config for " << ip << " in " << filename << endl;
	}
	
	system("chmod +x scripts/*");
	system("wait");
	
	for (size_t i = 0; i < files.size(); i++) {
		string call = "sshpass -p pass scp ";
		call += "-oStrictHostKeyChecking=no ";
		call += files.at(i);
		call += " data/testTone.wav root@";
		call += configs.at(i);
		call += ":/tmp/";
		call += " &";
		
		cout << "Executing system call: " << call << endl;
		
		system(call.c_str());
	}
	
	sleep(2);
	system("wait");
	
	for (auto& ip : configs) {
		string call = "sshpass -p pass ssh -oStrictHostKeyChecking=no root@";
		call += ip;
		call += " \'chmod +x /tmp/script";
		call += ip;
		call += ".sh && /tmp/script";
		call += ip;
		call += ".sh\' &";
		
		cout << "Running script: " << call << endl;
		
		if (RUN_SCRIPTS)
			system(call.c_str());
	}
	
	cout << "Scripts started, waiting for completion\n";
	
	if (RUN_SCRIPTS) {
		for (int i = 0; i < duration_seconds + 2; i++) {
			sleep(1);
			printf("%d/%d seconds elapsed (%1.0f%%)\n", (i + 1), duration_seconds + 2, (static_cast<double>(i + 1) / (duration_seconds + 2)) * 100.0);
		}
	}
	
	for (auto& ip : configs) {
		string call = "sshpass -p pass scp -oStrictHostKeyChecking=no root@";
		call += ip;
		call += ":/tmp/cap";
		call += ip;
		call += ".wav";
		call += " recordings/ &";
		
		cout << "Running script: " << call << endl;
		
		if (RUN_SCRIPTS)
			system(call.c_str());
	}
	
	cout << "Waiting for data transfer..\n";
	sleep(2);
	system("wait");
	
	return createFilenames(configs);
}

void writeResults(vector<Recording>& recordings) {
	ofstream file("../Calculated level 3");
	
	if (!file.is_open()) {
		cout << "Warning: could not open file for writing results\n";
		
		return;
	}
	
	for (size_t i = 0; i < recordings.size(); i++) {
		Recording& master = recordings.at(i);
		
		for (size_t j = i + 1; j < recordings.size(); j++) {
			Recording& slave = recordings.at(j);
			
			file << master.getLastIP() << " -> " << slave.getLastIP() << "\t= " << master.getDistance(j) << endl;
		}
	}
	
	file.close();
}

void writeLocalization(vector<Recording>& recordings) {
	ofstream file("../Localization/live_localization.txt");
	
	if (!file.is_open()) {
		cout << "Warning: could not open file for writing results\n";
		
		return;
	}
	
	file << to_string(recordings.size()) << endl;
	
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

int main(int argc, char** argv) {
	/*
		0: program path
		1: pause length in seconds
		2: recording speaker # 1 IP
		3: recording speaker # 2 IP
	*/
	
	/*
		script runs test tone at 4000 hz and 1 sec, 60000 samples
	*/	
	
	vector<Recording> recordings;
	
	if (argc < 2) {
		printHelp();
		ERROR("specify pause length");
	}
		
	g_playingLength = static_cast<int>((stod(argv[1]) * 48000 /* read this from file later */));
		
	if (argc <= 2) {
		printHelp();
		ERROR("specify input IP:s");
	}
		
	int num_recordings = argc - 2;
	
	vector<string> ips(argv + 2, argv + 2 + num_recordings);
	vector<string> filenames = runSetup(num_recordings, argv + 2);
	
	if (!RUN_SCRIPTS)
		return 1;
	
	//vector<string> ips = { "172.25.9.27", "172.25.9.38", "172.25.12.99", "172.25.12.168", "172.25.13.200", "172.25.13.250" };
	//vector<string> filenames = createFilenames(ips);
	
	for (int i = 0; i < num_recordings; i++) {
		string filename = filenames.at(i);
		
		recordings.push_back(Recording(ips.at(i)));
		
		Recording& recording = recordings.back();
		WavReader::read(filename, recording.getData());
		
		recording.findStartingTones(num_recordings, FREQ_N, FREQ_THRESHOLD, FREQ_REDUCING, FREQ_FREQ);
	}
	
	for (int i = 0; i < num_recordings; i++) {
		Recording& master = recordings.at(i);
		
		for (int j = i + 1; j < num_recordings; j++) {
			if (j == i)
				continue;
			
			Recording& recording = recordings.at(j);
			double distance = calculateDistance(master, recording);
			
			master.addDistance(j, distance);
			
			if (master.getDistance(j) > 1e01)
				ERROR("results did not pass sanity check, they are wrong");
					
			if (master.getDistance(j) < 1e04 /* sanity check */)
				cout << "Distance from " << master.getLastIP() << " -> " << recording.getLastIP() << " is "  << master.getDistance(j) << " m\n";
		}
	}
	
	writeResults(recordings);
	writeLocalization(recordings);
		
	return 0;
}