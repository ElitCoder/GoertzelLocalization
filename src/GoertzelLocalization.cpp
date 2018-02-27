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

int g_playingLength = 2e05;

size_t getEnding(size_t start) {
	return start + g_playingLength - 1;
}

size_t findPeak(vector<short>& data, size_t start) {
	double		dft;
	double		threshold = FREQ_THRESHOLD;
			
	while (threshold > 0) {
		for (size_t i = start; i < getEnding(start) - FREQ_N; i += FREQ_N) {
			dft = goertzel(FREQ_N, FREQ_FREQ, 48000 /* read this from wav file later */, data.data() + i) / static_cast<double>(SHRT_MAX);
						
			if (dft > threshold)
				return i;
		}
		
		threshold -= FREQ_REDUCING;
	}
	
	return 0;
}

void calculatePlacement(vector<Recording>& recordings) {
	recordings.front().setPosition({ 0, 0 });
}

double calculateDistance(Recording& master, Recording& recording) {
	long long	play_1 = 0;
	long long	play_2 = 0;
	long long	record_1 = 0;
	long long	record_2 = 0;
		
	play_1 = findPeak(master.getData(), master.getStartingPoint());
	play_2 = findPeak(recording.getData(), recording.getStartingPoint());
	record_1 = findPeak(master.getData(), recording.getStartingPoint());
	record_2 = findPeak(recording.getData(), master.getStartingPoint());
	
	long long	sum = (record_1 + record_2) - (play_1 + play_2);
	
	return abs(((double)sum/2))*343/48000;
}

template<class T>
static void plot(vector<T>& data) {
	vector<int>	tmp;
	for_each(data.begin(), data.end(), [&tmp] (T& element) { tmp.push_back(element); });
	
	plt::plot(tmp);
	plt::show();
}

/*
	create configs from ip
	send to speakers using scp (ssh -oStrictHostKeyChecking=no to skip fingerprints)
	start the scripts at the same time, how?
	wait until done
	scp to computer using scripted filename
	continue running this program
*/ 

void printHelp() {
	cout << "Usage: ./GoertzelLocalization <pause length in seconds> <speaker #1 IP> <speaker #2 IP>\n\n# of speakers is arbitrary\n\n1. Creates configuration files for every speaker based on IP, and sends"
		 <<	" the files using scp\n2. Starts the scripts at the same time\n3. Wait until done\n4. Retrieve all recordings from the speakers\n5. Calculate distances using Goertzel\n\n";
}

string createConfig(string& ip, int number, int duration) {
	string config = "";
	config += "cd tmp;\n";
	config += "\n";
	config += "systemctl stop audio*\n";
	config += "arecord -Daudiosource -r 48000 -fS16_LE -c1 -d";
	config += to_string(duration);
	config += " /tmp/cap";
	config += ip;
	config += ".wav &\n";
	config += "\n";
	config += "sleep ";
	config += to_string(g_playingLength / 48000 * (number + 1));
	config += "\n";
	config += "aplay -Dlocalhw_0 -r 48000 -fS16_LE /tmp/testTone.wav\n\nexit;\n";
	
	return config;
}

void runSetup(int num_recordings, char** ips) {
	vector<string> configs(ips, ips + num_recordings);
	int duration = g_playingLength /* until first tone */ + num_recordings * g_playingLength + g_playingLength /* to make up for jitter */;
	int duration_seconds = duration / 48000;
	
	cout << num_recordings << " speakers and " << g_playingLength << " pause length, writing " << duration_seconds << " as total duration\n";
	
	if (!system(NULL))
		ERROR("system calls are not available");
		
	system("mkdir scripts");
	
	for (size_t i = 0; i < configs.size(); i++) {
		string& ip = configs.at(i);
		
		string filename = "scripts/script";
		filename += ip;
		filename += ".sh";
		
		ofstream file(filename);
		
		if (!file.is_open()) {
			cout << "Warning: could not open file " << filename << endl;
			continue;
		}
		
		file << createConfig(ip, i, duration_seconds);
		
		file.close();
	}
	
	/*
	string config =
		"cd tmp;\n" +
		"\n" +
		"systemctl stop audio*\n" + 
		"arecord -Daudiosource -r 48000 -fS16_LE -c1 -d" + duration_seconds;
	*/	
}

int main(int argc, char** argv) {
	/*
		0: program path
		1: pause length in seconds
		2: recording speaker # 1 IP
		3: recording speaker # 2 IP
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
	
	runSetup(num_recordings, argv + 2);
	
	// just test runSetup()
	exit(1);
	
	size_t startingPoint = 2e05;
	
	for (int i = 0; i < num_recordings; i++) {
		string filename = argv[2 + i];
		
		recordings.push_back(Recording(filename));
		
		Recording& recording = recordings.back();
		WavReader::read(recording.getFilename(), recording.getData());
		
		if (i == 0) {
			startingPoint = recording.findActualStart(FREQ_N, FREQ_THRESHOLD, FREQ_REDUCING, FREQ_FREQ);
			recording.setStartingPoint(startingPoint);
		} else {
			startingPoint += g_playingLength;
			recording.setStartingPoint(startingPoint);
		}
		
		cout << "Set starting point to " << recording.getStartingPoint() << endl;
	}
	
	chrono::steady_clock::time_point begin = chrono::steady_clock::now();
	
	for (int i = 0; i < num_recordings; i++) {
		Recording& master = recordings.at(i);
		
		for (int j = 0; j < num_recordings; j++) {
			if (j == i)
			continue;
			
			Recording& recording = recordings.at(j);
			double distance = calculateDistance(master, recording);
			
			master.addDistance(j, distance);
				
			if (master.getDistance(j) < 1e04 /* sanity check */)
				cout << "Distance from " << i + 1 << " -> " << j + 1 << " is "  << master.getDistance(j) << " m\n";
		}
	}
	
	cout << "Algorithm execution time: " << static_cast<double>(std::chrono::duration_cast<std::chrono::nanoseconds>(chrono::steady_clock::now() - begin).count()) / 1e09 << " s\n";
		
	return 0;
}