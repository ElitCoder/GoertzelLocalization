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

int g_playingLength = 2e05;

size_t getEnding(size_t start) {
	return start + g_playingLength - 1;
}

size_t findPeak(vector<short>& data, size_t start) {
	const int	N = 16;
	double		dft;
	double		threshold = 0.01;
			
	while (threshold > 0) {
		for (size_t i = start; i < getEnding(start) - N; i += N) {
			dft = goertzel(N, 4000, 48000, data.data() + i) / 32767.0;
						
			if (dft > threshold)
				return i;
		}
		
		threshold -= 0.001;
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
void plot(vector<T>& data) {
	vector<int>	tmp;
	for_each(data.begin(), data.end(), [&tmp] (T& element) { tmp.push_back(element); });
	
	plt::plot(tmp);
	plt::show();
}

int main(int argc, char** argv) {
	/*
		0: program path
		1: # of recordings
		2: recording # 1 filename
		3: recording # 1 starting point
		?: custom length
	*/
	
	vector<Recording> recordings;
	
	if (argc < 2)
		ERROR("specify recordings");
		
	int num_recordings = stoi(argv[1]);		
	
	if (argc < 2 + num_recordings * 2)
		ERROR("missing parameters for recordings");
	
	for (int i = 0; i < num_recordings * 2; i += 2) {
		string filename = argv[2 + i];
		int start = stoi(argv[2 + i + 1]);
		
		recordings.push_back(Recording(filename, start));
		
		Recording& recording = recordings.back();
		WavReader::read(recording.getFilename(), recording.getData());
	}
	
	if (argc >= 2 + num_recordings * 2 + 1) {
		cout << "Setting custom length\n";
		g_playingLength = stoi(argv[2 + num_recordings * 2 + 1]);
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
				
			if (master.getDistance(j) < 1e21)
				cout << "Distance from " << i + 1 << " -> " << j + 1 << " is "  << master.getDistance(j) << " m\n";
		}
	}
	
	cout << "Execution time: " << static_cast<double>(std::chrono::duration_cast<std::chrono::nanoseconds>(chrono::steady_clock::now() - begin).count()) / 1e09 << " s\n";
		
	return 0;
}