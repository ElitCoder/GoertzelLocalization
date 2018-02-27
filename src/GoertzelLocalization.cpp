#include "WavReader.h"
#include "Goertzel.h"
#include "Recording.h"

#include "matplotlibcpp.h"

#include <iostream>
#include <algorithm>
#include <chrono>

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

int main(int argc, char** argv) {
	/*
		0: program path
		1: pause length in seconds
		2: # of recordings
		3: recording # 1 filename
	*/
	
	vector<Recording> recordings;
	
	if (argc < 2)
		ERROR("specify pause length");
		
	g_playingLength = static_cast<int>((stod(argv[1]) * 48000 /* read this from file later */));
	
	if (argc < 3)
		ERROR("specify number of recordings");
		
	int num_recordings = stoi(argv[2]);		
	
	if (argc < 3 + num_recordings)
		ERROR("missing parameters for recordings");
	
	size_t startingPoint = 2e05;
	
	for (int i = 0; i < num_recordings; i++) {
		string filename = argv[3 + i];
		
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
	
	cout << "Execution time: " << static_cast<double>(std::chrono::duration_cast<std::chrono::nanoseconds>(chrono::steady_clock::now() - begin).count()) / 1e09 << " s\n";
		
	return 0;
}