#include "WavReader.h"
//#include "Goertzel.h"
#include "Recording.h"
#include "Matrix.h"
#include "DeltaContainer.h"
#include "Settings.h"
#include "Run.h"

#include <libnessh/SSHMaster.h>

#include <iostream>
#include <algorithm>
#include <chrono>
#include <fstream>
#include <sstream>
#include <cmath>

#define EPSILON	(0.01)

#define ERROR(...)	do { fprintf(stderr, "Error: "); fprintf(stderr, __VA_ARGS__); fprintf(stderr, "\n"); exit(1); } while(0)

using namespace std;

enum RUN_TYPES {
	RUN_GOERTZEL,
	RUN_NOTHING,
	JUST_RUN_FULL,
	JUST_RUN_SIMPLE
};

static Settings g_settings;

int g_playingLength = 2e05;

/* Not currently used
static bool equal(double a, double b) {
	return abs(a - b) < EPSILON;
}

static bool isNaN(double a) {
	return a != a;
}
*/

double calculateDistance(Recording& master, Recording& recording) {
	long long r12 = recording.getTonePlayingWhen(master.getId());
	long long p1 = master.getTonePlayingWhen(master.getId());
	long long r21 = master.getTonePlayingWhen(recording.getId());
	long long p2 = recording.getTonePlayingWhen(recording.getId());
	
	//T12 = Tp + Dt
	//T21 = Tp - Dt
	double T12 = r12 - p1;
	double T21 = r21 - p2;
	
	double Dt = -(static_cast<double>(T21) - static_cast<double>(T12)) / 2;
	
	double Tp1 = T12 - Dt;
	double Tp2 = T21 + Dt;
	
	double Tp = (Tp1 + Tp2) / 2;
	double Tp_sec = Tp / 48000;
	
	return abs(Tp_sec * 343);
	
	#if 0
	
	double correct_Tp12 = -1;
	double correct_Tp21 = -1;
	double min_difference = 1000000000;
	double correct_dt = -1;
	
	vector<double> zeroes;
	
	cout << "Original Tp12: " << T12 - Dt << endl;
	cout << "Original Tp21: " << T21 + Dt << endl;
	
	for (double d12 = -10000; d12 < 10000; d12 += 0.5) {
		for (double d21 = -10000; d21 < 10000; d21 += 0.5) {
			double Tp12 = sqrt((T12 * T12) - (T12 * d12) + (d12 * d12));
			double Tp21 = sqrt((T21 * T21) - (T21 * d21) + (d21 * d21));
			
			//double Tp12 = T12 - d12;
			//double Tp21 = T21 - d21;
					
			if (isNaN(Tp12) || isNaN(Tp21) || Tp12 < 0 || Tp21 < 0)
				continue;
				
			double difference = abs(Tp12 - Tp21);
			
			//cout << "iteration distance: " << Tp12 / 48000 * 343 << endl;
			
			if (equal(difference, 0)) {
				zeroes.push_back((Tp12 + Tp21) / 2);
				
				//cout << "Zero at: " << d12 << " " << d21 << endl;
				//cout << "Tp12: " << Tp12 << " Tp21: " << Tp21 << endl;
				
				//cout << "deltas " << d12 / 48000 << " " << d12 / 48000 << endl;
			}
		
			if (difference < min_difference) {
				correct_Tp12 = Tp12;
				correct_Tp21 = Tp21;
				correct_dt = d12;
				
				//cout << master.getId() << " found minimal at " << d12 << " and " << d21 << endl;
				//cout << master.getId() << " which is " << difference << endl;
				
				min_difference = difference;
			}
		}
	}
	
	cout << "Zeroes: " << zeroes.size() << endl;
	cout << "Min diff: " << min_difference / 48000 << endl;
	cout << "Original Dt: " << Dt << endl;
	cout << "Calculated Dt: " << correct_dt << endl;
	
	correct_Tp12 /= 48000;
	correct_Tp21 /= 48000;
	
	correct_Tp12 *= 343;
	correct_Tp21 *= 343;
	
	for (size_t i = 0; i < zeroes.size(); i++) {
		cout << "Solution " << (i + 1) << " " << (zeroes.at(i) / 48000) * 343 << endl;
	}
	
	//cout << endl;
	
	master.setFrameDistance(recording.getId(), FIRST, T12);
	//cout << "set id " << master.getId() << " to " << recording.getId() << endl;
	master.setFrameDistance(recording.getId(), SECOND, T21);
	
	;

	//cout << "Versus simple solution at " << (((T12 - Dt) + (T21 + Dt)) / 2) / 48000 * 343 << endl;
	//cout << "With delta " << Dt / 48000 << endl;
	
	//return (correct_Tp12 + correct_Tp21) / 2;
	
	//T12 - Dt = Tp
	//T21 + Dt = Tp
	
	#endif
}

/*
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
*/

void printHelp() {
	/*
	cout << "Usage: ./GoertzelLocalization <pause length in seconds> <speaker #1 IP> <speaker #2 IP>\n\n# of speakers is arbitrary\n\n1. Creates configuration files for every speaker based on IP, and sends"
		 <<	" the files using scp\n2. Starts the scripts at the same time\n3. Wait until done\n4. Retrieve all recordings from the speakers\n5. Calculate distances using Goertzel\n\n";
	*/
	
	cout << "Usage: DistanceCalculation [-options]\n\n";
	cout << "Runs the localization test using Goertzel algorithm to detect sound.\n\n";
	cout << "Options:\n";
	cout << "\t-p,\t\t specify pause length in seconds between beeps\n";
	cout << "\t-er,\t\t specify extra recording length, will be added after the beeps\n";
	cout << "\t-h,\t\t print this help text\n";
	cout << "\t-f,\t\t specify file with IPs, with the format of one IP address per line\n";
	cout << "\t-t,\t\t specify which mode to run, default is GOERTZEL, other modes are NOTHING (just collects the sound samples)\n";
	cout << "\t-tf,\t\t specify the test file to run (place it in data/, should be 1 s 48 kHZ, default is testTone.wav)\n";
	cout << "\t-jr,\t\t don't run the full script, just read the values from recordings/ directly (default is FALSE)\n";
}

string createConfig(string& ip, int number, int duration) {
	int extra_record = 0;
	
	if (g_settings.has("-er"))
		extra_record = g_settings.get<int>("-er");
		
	string config = "";
	config += "systemctl stop audio*\n";
	config += "arecord -Daudiosource -r 48000 -fS16_LE -c1 -d";
	config += to_string(duration + extra_record);
	config += " /tmp/cap";
	config += ip;
	config += ".wav &\n";
	config += "\n";
	config += "sleep ";
	config += to_string(1 + (g_playingLength / 48000 * (number + 1)));
	config += "\n";
	//config += "aplay -Dlocalhw_0 -r 48000 -fS16_LE /tmp/testTone.wav\n\nexit;\n";
	config += "aplay -Dlocalhw_0 -r 48000 -f S16_LE /tmp/";
	config += (g_settings.has("-tf") ? g_settings.get<string>("-tf") : "testTone.wav");
	config += "\n\nexit;\n";
	
	return config;
}

vector<string> createFilenames(vector<string>& configs) {
	vector<string> filenames;
	
	for (auto& ip : configs) {
		string filename = "recordings/cap";
		//string filename = "../backups/level_3_distances/recordings/cap";
		filename += ip;
		filename += ".wav";
		
		filenames.push_back(filename);
		
		//cout << "Creating filename: " << filename << endl;
	}
	
	return filenames;
}

/*
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
*/

/*
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
*/

vector<string> runSetup(const vector<string>& ips) {//int num_recordings, char** ips) {
	SSHMaster ssh_master;
	
	vector<string> configs(ips);//ips, ips + num_recordings);
	vector<string> files;
	int num_recordings = ips.size();
	int extra_record = 0;
	
	if (g_settings.has("-er"))
		extra_record = g_settings.get<int>("-er");
		
	int duration = g_playingLength /* until first tone */ + num_recordings * g_playingLength + g_playingLength /* to make up for jitter */;
	int duration_seconds = (duration / 48000);
	
	cout << "Using " << (g_settings.has("-tf") ? g_settings.get<string>("-tf") : "testTone.wav") << " as test file\n";
	cout << "Connecting to speakers using SSH.. ";
	
	if (!ssh_master.connect(configs, "pass"))
		ERROR("ssh connection failed");
		
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
	
	vector<string> from;
	vector<string> to;
	
	for (size_t i = 0; i < files.size(); i++) {
		//string file = "data/testTone.wav " + files.at(i);
		string file = "data/";
		file += (g_settings.has("-tf") ? g_settings.get<string>("-tf") : "testTone.wav");
		file += " ";
		file += files.at(i);
		
		from.push_back(file);
		to.push_back("/tmp/");
	}
	
	if (!ssh_master.transferRemote(configs, from, to))
		ERROR("error transferring scripts and test tones");
	
	cout << "done\n";
	
	cout << "Running scripts at remotes.. ";
	
	vector<string> commands;
	
	for (auto& ip : configs)
		commands.push_back("chmod +x /tmp/script" + ip + ".sh; /tmp/script" + ip + ".sh");
	
	if (!ssh_master.command(configs, commands))
		ERROR("could not command ssh");

	cout << "done, waiting for finish\n";
	
	for (int i = 0; i < duration_seconds + 1 + extra_record; i++) {
		sleep(1);
		printf("%d/%d seconds elapsed (%1.0f%%)\n", (i + 1), duration_seconds + extra_record + 1, (static_cast<double>(i + 1) / (duration_seconds + extra_record + 1)) * 100.0);
	}
	
	cout << "Downloading recordings.. ";
	
	//vector<string> from;
	//vector<string> to;
	from.clear();
	to.clear();
	
	for (auto& ip : configs) {
		from.push_back("/tmp/cap" + ip + ".wav");
		to.push_back("recordings");
	}
		
	if (!ssh_master.transferLocal(configs, from, to, true))
		ERROR("could not retrieve recordings");
	
	cout << "done\n";
	
	cout << "Creating filenames.. ";
	auto filenames = createFilenames(configs);
	cout << "done\n\n";
	
	return filenames;
}

bool checkParameters(int argc, char** argv) {
	argc--;
	argv++;
	
	g_settings.set("-h", "0");
	
	for (int i = 0; i < argc; i += 2) {
		string option = string(argv[i]);
		
		if (i + 1 >= argc)
			return false;
			
		string value = argv[i + 1];
		
		g_settings.set(option, value);
	}
	
	if (!g_settings.has("-f")) {
		cout << "Error: no file specified\n\n";
		
		return false;
	}
	
	return true;
}

vector<string> readIps(const string& filename) {
	ifstream file(filename);
	
	if (!file.is_open())
		ERROR("could not open file %s", filename.c_str());
		
	vector<string> ips;
	string ip;	
		
	while (getline(file, ip))
		ips.push_back(ip);
		
	file.close();	
		
	return ips;	
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
	
	if (!checkParameters(argc, argv) || g_settings.get<bool>("-h")) {
		printHelp();
		
		return -1;
	}
	
	if (g_settings.has("-p"))
		g_playingLength = g_settings.get<int>("-p") * 48000;
	else
		g_playingLength = 2 * 48000;
	
	vector<Recording> recordings;
	
	vector<string> ips = readIps(g_settings.get<string>("-f"));
	
	int setting_js = JUST_RUN_FULL;
	
	if (g_settings.has("-js"))
		if (g_settings.get<string>("-js") == "TRUE")
			setting_js = JUST_RUN_SIMPLE;
		
	vector<string> filenames = setting_js == JUST_RUN_FULL ? runSetup(ips) : createFilenames(ips);
		
	int run_type = RUN_GOERTZEL;
		
	if (g_settings.has("-t")) {
		string type = g_settings.get<string>("-t");
		
		// Add more run types here
		if (type == "NOTHING")
			run_type = RUN_NOTHING;
	}
	
	switch (run_type) {
		case RUN_GOERTZEL: Run::runGoertzel(filenames, ips);
			break;
			
		case RUN_NOTHING:
			break;
			
		default: ERROR("no running mode specified");
			break;
	}
		
	return 0;
}