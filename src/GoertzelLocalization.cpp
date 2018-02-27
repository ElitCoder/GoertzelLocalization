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
#if 0

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
#endif

double calculateDistance(Recording& master, Recording& recording) {
	long long	play_1 = 0;
	long long	play_2 = 0;
	long long	record_1 = 0;
	long long	record_2 = 0;
	
	/*
	play_1 = findPeak(master.getData(), master.getStartingPoint());
	play_2 = findPeak(recording.getData(), recording.getStartingPoint());
	record_1 = findPeak(master.getData(), recording.getStartingPoint());
	record_2 = findPeak(recording.getData(), master.getStartingPoint());
	*/
	
	play_1 = master.getTonePlayingWhen(master.getId());
	play_2 = recording.getTonePlayingWhen(recording.getId());
	record_1 = master.getTonePlayingWhen(recording.getId());
	record_2 = recording.getTonePlayingWhen(master.getId());
	
	/*
	cout << "play_1: " << play_1 << endl;
	cout << "play_2: " << play_2 << endl;
	cout << "record_1: " << record_1 << endl;
	cout << "record_2: " << record_2 << endl;
	*/
	
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
	
	cout << "Setting permissions for scripts..\n";
	
	system("chmod +x scripts/*");
	sleep(1);
	system("wait");
	
	for (size_t i = 0; i < files.size(); i++) {
		string call = "sshpass -p pass scp ";
		call += "-oStrictHostKeyChecking=no ";
		call += files.at(i);
		call += " root@";
		call += configs.at(i);
		call += ":/tmp/";
		call += " &";
		
		cout << "Executing system call: " << call << endl;
		
		system(call.c_str());
	}
	
	cout << "Waiting for system call completion..\n";
	sleep(1);
	system("wait");
	
	for (size_t i = 0; i < files.size(); i++) {
		string call = "sshpass -p pass ssh -oStrictHostKeyChecking=no root@";
		call += configs.at(i);
		call += " \'chmod +x ";
		call += "/tmp/script";
		call += configs.at(i);
		call += ".sh\' &";
		
		cout << "Executing system call: " << call << endl;
		
		system(call.c_str());
	}
	
	cout << "Waiting for system call completion..\n";
	sleep(1);
	system("wait");
	cout << "Transferring testTone.wav..\n";
	
	for (auto& ip : configs) {
		string call = "sshpass -p pass scp -oStrictHostKeyChecking=no data/testTone.wav root@";
		call += ip;
		call += ":/tmp/ &";
		
		cout << "Executing system call: " << call << endl;
		
		system(call.c_str());
	}
	
	cout << "Waiting for system call completion..\n";
	sleep(1);
	system("wait");
	
	for (auto& ip : configs) {
		string call = "sshpass -p pass ssh -oStrictHostKeyChecking=no root@";
		call += ip;
		call += " \'/tmp/script";
		call += ip;
		call += ".sh\' &";
		
		cout << "Running script: " << call << endl;
		
		if (RUN_SCRIPTS)
			system(call.c_str());
	}
	
	cout << "Scripts started, waiting for completion\n";
	
	if (RUN_SCRIPTS) {
		for (int i = 0; i < duration_seconds + 1; i++) {
			sleep(1);
			printf("%d/%d seconds elapsed (%1.0f%%)\n", (i + 1), duration_seconds + 1, (static_cast<double>(i + 1) / (duration_seconds + 1)) * 100.0);
		}
	}
	
	cout << "Scripts executed hopefully, collecting data into recordings/\n";
	sleep(1);
	system("wait");
	
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
	
	cout << "Creating filenames for algorithm\n";
	
	return createFilenames(configs);
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
		string filename = filenames.at(i);//argv[2 + i];
		
		recordings.push_back(Recording(filename, ips.at(i)));
		
		Recording& recording = recordings.back();
		WavReader::read(recording.getFilename(), recording.getData());
		
		recording.findStartingTones(num_recordings, FREQ_N, FREQ_THRESHOLD, FREQ_REDUCING, FREQ_FREQ);
		
		/*
		if (i == 0) {
			startingPoint = recording.findActualStart(FREQ_N, FREQ_THRESHOLD, FREQ_REDUCING, FREQ_FREQ);
			recording.setStartingPoint(startingPoint);
		} else {
			startingPoint += g_playingLength;
			recording.setStartingPoint(startingPoint);
		}
		*/
		
		//cout << "Set starting point to " << recording.getStartingPoint() << endl;
		
		// ignore plot if matplotlibcpp fails
		/*
		try {
			plot(recording.getData());
		} catch(...) {
			continue;
		}
		*/
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
				cout << "Distance from " << master.getLastIP() << " -> " << recording.getLastIP() << " is "  << master.getDistance(j) << " m\n";
		}
	}
	
	cout << "Algorithm execution time: " << static_cast<double>(std::chrono::duration_cast<std::chrono::nanoseconds>(chrono::steady_clock::now() - begin).count()) / 1e09 << " s\n";
		
	return 0;
}