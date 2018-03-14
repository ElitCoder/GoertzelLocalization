#include "WavReader.h"
#include "Goertzel.h"
#include "Recording.h"
#include "Connections.h"
#include "Matrix.h"
#include "DeltaContainer.h"

#include <iostream>
#include <algorithm>
#include <chrono>
#include <fstream>
#include <sstream>
#include <cmath>

#define EPSILON	(0.001)

#define ERROR(...)	do { fprintf(stderr, "Error: "); fprintf(stderr, __VA_ARGS__); fprintf(stderr, "\n"); exit(1); } while(0)

using namespace std;

static const int FREQ_N = 16;
static const int FREQ_FREQ = 4000;
static const double FREQ_REDUCING = 0.001;
static const double FREQ_THRESHOLD = 0.05;//0.01;

static bool RUN_SCRIPTS = true;

int g_playingLength = 2e05;

/*
static bool equal(double a, double b) {
	return abs(a - b) < EPSILON;
}
*/

double calculateDistance(Recording& master, Recording& recording) {
	long long r12 = recording.getTonePlayingWhen(master.getId());
	long long p1 = master.getTonePlayingWhen(master.getId());
	long long r21 = master.getTonePlayingWhen(recording.getId());
	long long p2 = recording.getTonePlayingWhen(recording.getId());
	
	//T12 = Tp + Dt
	//T21 = Tp - Dt
	long long T12 = r12 - p1;
	long long T21 = r21 - p2;
	
	double Dt = -(static_cast<double>(T21) - static_cast<double>(T12)) / 2;
	
	//T12 - Dt = Tp
	//T21 + Dt = Tp
	double Tp1 = T12 - Dt;
	double Tp2 = T21 + Dt;
	
	double Tp = (Tp1 + Tp2) / 2;
	double Tp_sec = Tp / 48000;
	
	master.setFrameDistance(recording.getId(), FIRST, T12);
	//cout << "set id " << master.getId() << " to " << recording.getId() << endl;
	master.setFrameDistance(recording.getId(), SECOND, T21);
	
	return abs(Tp_sec * 343);
}

double calculateDistance(Recording& master, Recording& recording, double delta) {
	long long r12 = recording.getTonePlayingWhen(master.getId());
	long long p1 = master.getTonePlayingWhen(master.getId());
	long long r21 = master.getTonePlayingWhen(recording.getId());
	long long p2 = recording.getTonePlayingWhen(recording.getId());
	
	long long T12 = r12 - p1;
	long long T21 = r21 - p2;
	
	delta *= 48000;
	
	double Tp = sqrt((((T12 * T12) + (T21 * T21)) / 2) - (delta * delta));
	
	return (Tp / 48000) * 343;
}

Matrix<double> calculateDeltas(const vector<Recording>& recordings) {
	Matrix<double> matrix(recordings.size(), recordings.size());

	//Txy = rxy - px
	//Tyx = ryx - py
	//(i, j) = Tij = rij - pi
	//(i, j) = j.tone(i) - i.tone(i)
	
	for (size_t i = 0; i < recordings.size(); i++) {
		for (size_t j = 0; j < recordings.size(); j++) {
			const Recording& i_recording = recordings.at(i);
			const Recording& j_recording = recordings.at(j);
			
			long long rij = j_recording.getTonePlayingWhen(i_recording.getId());
			long long pi = i_recording.getTonePlayingWhen(i_recording.getId());
			
			matrix[i][j] = (rij - pi) / static_cast<double>(48000);
		}
	}
	
	// D = (1/2) * (M - M^T) 
	return (matrix - matrix.transpose()) * 0.5;
}

vector<Matrix<double>> compareDeltaDifferences(const Matrix<double>& deltas) {
	vector<Matrix<double>> differences;
	
	for (size_t i = 0; i < deltas.size(); i++) {
		differences.push_back(Matrix<double>(deltas.size(), deltas.size()));
		auto& difference = differences.back();
		
		for (size_t j = 0; j < deltas.size() - 1; j++) {
			if (i == j)
				continue;
				
			size_t choose_second_j = j + 1 == i ? j + 2 : j + 1;
			
			if (choose_second_j >= deltas.size())
				break;
			
			//cout << "i: " << i << " j: " << j << " second_j: " << choose_second_j << endl;
			difference[choose_second_j][j] = deltas[i][j] - deltas[i][choose_second_j];
			difference[j][choose_second_j] = -difference[choose_second_j][j];
			
			//printf("saving in [%zu][%zu]\n", choose_second_j, j);
		}
	}
	
	return differences;
}

vector<DeltaContainer> getDeltaValues(const Matrix<double>& deltas, const vector<Matrix<double>>& differences) {
	vector<DeltaContainer> containers;
	
	for (size_t i = 0; i < deltas.size(); i++) {
		for (size_t j = 0; j < deltas.size(); j++) {
			if (i == j)
				continue;
								
			containers.push_back(DeltaContainer(i, j));
		}
	}
		
	for (size_t i = 0; i < deltas.size(); i++) {
		const auto& difference = differences.at(i);
		
		for (size_t j = 0; j < deltas.size(); j++) {
			if (i == j)
				continue;
			
			size_t choose_second_j = j + 1 == i ? j + 2 : j + 1;
			
			if (choose_second_j >= deltas.size())
				break;	
								
			DeltaContainer& container_positive = *find(containers.begin(), containers.end(), DeltaContainer(choose_second_j, j));
			DeltaContainer& container_negative = *find(containers.begin(), containers.end(), DeltaContainer(j, choose_second_j));
			
			container_positive.add(difference[choose_second_j][j]);
			container_negative.add(difference[j][choose_second_j]);
		}
	}
		
	for (size_t i = 0; i < deltas.size(); i++) {
		for (size_t j = 0; j < deltas.size(); j++) {
			if (i == j)
				continue;
				
			DeltaContainer& container = *find(containers.begin(), containers.end(), DeltaContainer(i, j));
			container.add(deltas[i][j]);	
		}
	}
	
	return containers;
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
		
		//cout << "Creating filename: " << filename << endl;
	}
	
	return filenames;
}

static vector<string> splitString(const string& input, char split) {
	vector<string> splitted;
	size_t last_split = 0;
	
	for (size_t i = 0; i < input.length(); i++) {
		if (input.at(i) == split) {
			splitted.push_back(input.substr(last_split, i - last_split));
			last_split = i + 1;
		}
	}
	
	// Don't forget last token
	if (last_split != input.length()) {
		splitted.push_back(input.substr(last_split, input.length() - last_split));
	}
	
	return splitted;
}

vector<double> calculateRealDifference(const string& real, const string& simulated) {
	vector<double> differences;
	
	string real_line;
	string simulated_line;
	stringstream real_stream(real);
	stringstream simulated_stream(simulated);
	
	while (getline(real_stream, real_line, '\n') && getline(simulated_stream, simulated_line, '\n')) {
		auto real_tokens = splitString(real_line, ' ');
		auto simulated_tokens = splitString(simulated_line, ' ');
		
		double difference = abs(stod(real_tokens.back()) - stod(simulated_tokens.back()));
		
		cout << "Difference (" << real_tokens.front() << " -> " << real_tokens.at(2) << ")\t= " << difference << endl;
		differences.push_back(difference);
	}
	
	return differences;
}

vector<double> calculateRealDifference(ifstream& real, ifstream& simulated) {
	if (!real.is_open() || !simulated.is_open()) {
		cout << "Warning: real or simulated is not open (files)\n";
		
		return vector<double>();
	}
	
	stringstream real_stream;
	real_stream << real.rdbuf();
	
	stringstream simulated_stream;
	simulated_stream << simulated.rdbuf();
	
	real.close();
	simulated.close();
	
	return calculateRealDifference(real_stream.str(), simulated_stream.str());
}

vector<string> runSetup(int num_recordings, char** ips) {
	Connections ssh_master;
	
	vector<string> configs(ips, ips + num_recordings);
	vector<string> files;
	int duration = g_playingLength /* until first tone */ + num_recordings * g_playingLength + g_playingLength /* to make up for jitter */;
	int duration_seconds = duration / 48000;
	
	cout << "Connecting to speakers using SSH.. ";
	ssh_master.connect(configs, "pass");
	cout << "done\n";
	
	if (!system(NULL))
		ERROR("system calls are not available");
		
	int call = system("mkdir scripts; wait");
	call = system("mkdir recordings; wait");
	
	if (call) {} // Hide warnings
	
	cout << "Creating config files.. ";
	
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
	}
	
	call = system("chmod +x scripts/*; wait");
	
	cout << "done\n";
	
	cout << "Transferring scripts and test files.. ";
	
	if (RUN_SCRIPTS) {
		vector<string> from;
		vector<string> to;
		
		for (size_t i = 0; i < files.size(); i++) {
			string file = "data/testTone.wav " + files.at(i);
			
			from.push_back(file);
			to.push_back("/tmp/");
		}
		
		ssh_master.transferRemote(configs, from, to);
	}
	
	cout << "done\n";
	
	cout << "Running scripts at remotes.. ";
	
	vector<string> commands;
	
	for (auto& ip : configs)
		commands.push_back("chmod +x /tmp/script" + ip + ".sh; /tmp/script" + ip + ".sh");
	
	if (RUN_SCRIPTS)
		ssh_master.command(configs, commands);
	
	cout << "done, waiting for finish\n";
	
	if (RUN_SCRIPTS) {
		for (int i = 0; i < duration_seconds + 1; i++) {
			sleep(1);
			printf("%d/%d seconds elapsed (%1.0f%%)\n", (i + 1), duration_seconds + 1, (static_cast<double>(i + 1) / (duration_seconds + 1)) * 100.0);
		}
	}
	
	if (RUN_SCRIPTS) {
		cout << "Downloading recordings.. ";
		
		vector<string> from;
		vector<string> to;
		
		for (auto& ip : configs) {
			from.push_back("/tmp/cap" + ip + ".wav");
			to.push_back("recordings/");
		}
			
		ssh_master.transferLocal(configs, from, to, true);
		
		cout << "done\n";
	}
	
	cout << "Creating filenames.. ";
	auto filenames = createFilenames(configs);
	cout << "done\n\n";
	
	return filenames;
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
	ofstream file("../Localization3D/live_localization.txt");
	
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
	
	if (string(argv[1]) == "-X") {
		if (argc < 4)
			ERROR("specify input files as ./program -X <real> <simulated>");
			
		string real_file = argv[2];
		string simulated_file = argv[3];
		
		ifstream real(real_file);
		ifstream simulated(simulated_file);
		
		auto differences = calculateRealDifference(real, simulated);
		
		return 0;
	}
	
	g_playingLength = static_cast<int>((stod(argv[1]) * 48000 /* read this from file later */));
		
	if (argc <= 2) {
		printHelp();
		ERROR("specify input IP:s");
	}
		
	int num_recordings = argc - 2;
	
	vector<string> ips(argv + 2, argv + 2 + num_recordings);
	vector<string> filenames = runSetup(num_recordings, argv + 2);
	//vector<string> filenames = createFilenames(ips);
	
	if (!RUN_SCRIPTS)
		return 1;
	
	for (int i = 0; i < num_recordings; i++) {
		string filename = filenames.at(i);
		
		recordings.push_back(Recording(ips.at(i)));
		
		Recording& recording = recordings.back();
		WavReader::read(filename, recording.getData());
		
		recording.findStartingTones(num_recordings, FREQ_N, FREQ_THRESHOLD, FREQ_REDUCING, FREQ_FREQ);
	}
	
	cout << endl;
	
	for (int i = 0; i < num_recordings; i++) {
		Recording& master = recordings.at(i);
		
		for (int j = 0; j < num_recordings; j++) {
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
	
	/*
	for (size_t i = 0; i < recordings.size(); i++) {
		auto& recording = recordings.at(i);
		
		for (size_t j = 0; j < recordings.size(); j++) {
			if (i == j)
				continue;
				
			cout << i << " find " << j << " at " << recording.getTonePlayingWhen(j) / (double)48000 << " s\n";
			cout << i << " to " << j << " takes " << to_string(recording.getFrameDistance(j, FIRST) / (double)48000) << " s\n";
			cout << j << " to " << i << " takes " << recording.getFrameDistance(j, SECOND) / (double)48000 << " s\n";
		}
	}
	*/
	
	//cout << endl;	
	
	auto deltas = calculateDeltas(recordings);
	auto delta_differences = compareDeltaDifferences(deltas);
	auto delta_values = getDeltaValues(deltas, delta_differences);
	
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
	
	writeResults(recordings);
	writeLocalization(recordings);
	writeLocalization3D(recordings);
		
	return 0;
}