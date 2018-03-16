#include "Recording.h"
#include "Goertzel.h"

#include <iostream>
#include <sstream>
#include <climits>

#define ERROR(...)	do { fprintf(stderr, "Error: "); fprintf(stderr, __VA_ARGS__); fprintf(stderr, "\n"); exit(1); } while(0)

using namespace std;

extern int g_playingLength;

Recording::Recording(const string& ip) :
	ip_(ip) {
	static int id;
	id_ = id++;
}

vector<short>& Recording::getData() {
	return data_;
}

void Recording::addDistance(int id, double distance) {
	distances_.push_back({ id, distance});
}

void Recording::setDistance(int id, double distance) {
	for (auto& pair : distances_)
		if (pair.first == id)
			pair.second = distance;
}

double Recording::getDistance(int id) {
	for (size_t i = 0; i < distances_.size(); i++)
		if (distances_.at(i).first == id)
			return distances_.at(i).second;
	
	cout << "Warning: could not find client\n";		
	return 0;		
}

void Recording::setPosition(const std::pair<double, double>& position) {
	position_ = position;
}

void Recording::findStartingTones(int num_recordings, const int N, double threshold, double reducing, int frequency) {
	double dft;
	double orig_threshold = threshold;
	size_t current_i = g_playingLength + 0.5 * (double)48000; // Should start at g_playingLength + 0.5 sec
	size_t frame_ending = 48000 + (g_playingLength * (0 + 1)) + 1.5 * (double)48000;
	
	//cout << "Checking first tone at " << current_i / (double)48000 << " to " << frame_ending / (double)48000 << "\n";
	
	for (int i = 0; i < num_recordings; i++) {
		threshold = orig_threshold;
		
		while (threshold > 0) {
			size_t delta_current = current_i;
			bool found = false;
			
			for (; delta_current < frame_ending; delta_current++) {
				dft = goertzel(N, frequency, 48000, data_.data() + delta_current) / static_cast<double>(SHRT_MAX);
				
				if (dft > threshold) {
					starting_tones_.push_back(delta_current);
					current_i = (g_playingLength * (i + 2)) + 0.5 * (double)48000;
					frame_ending = 48000 + (g_playingLength * (i + 2)) + 1.5 * (double)48000;
					
					//cout << "Found tone, checking " << current_i / (double)48000 << " to " << frame_ending / (double)48000 << " now\n";
					
					found = true;
					break;
				}
			}
			
			if (found)
				break;
				
			threshold -= reducing;	
		}
	}
}

size_t Recording::getTonePlayingWhen(int id) const {
	if (static_cast<unsigned int>(id) >= starting_tones_.size())
		ERROR("not all tones were recorded, aborting");
		
	return starting_tones_.at(id);
}

int Recording::getId() const {
	return id_;
}

string Recording::getIP() {
	return ip_;
}

string Recording::getLastIP() const {
	istringstream stream(ip_);
	vector<string> tokens;
	string token;
	
	while (getline(stream, token, '.'))
		if (!token.empty())
			tokens.push_back(token);
	
	return tokens.back();
}

void Recording::setFrameDistance(int id, int which, long long distance) {
	switch (which) {
		case FIRST: distances_first_.push_back({ id, distance });
			break;
			
		case SECOND: distances_second_.push_back({ id, distance });
			break;
	}
}

long long Recording::getFrameDistance(int id, int which) {
	vector<pair<int, double>>* distances = &distances_first_;
	
	if (which == SECOND)
		distances = &distances_second_;
		
	for (size_t i = 0; i < distances->size(); i++)
		if (distances->at(i).first == id)
			return distances->at(i).second;
			
	ERROR("did not find frame distance, which: %d", which);
			
	return -1;		
}