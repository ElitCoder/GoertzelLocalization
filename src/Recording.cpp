#include "Recording.h"
#include "Goertzel.h"

#include <iostream>
#include <sstream>
#include <climits>

#define ERROR(...)	do { fprintf(stderr, "Error: "); fprintf(stderr, __VA_ARGS__); fprintf(stderr, "\n"); exit(1); } while(0)

using namespace std;

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
	size_t current_i = 0;
	
	for (int i = 0; i < num_recordings; i++) {
		threshold = orig_threshold;
		
		while (threshold > 0) {
			bool found = false;
			
			for (; current_i < data_.size(); current_i++) {
				dft = goertzel(N, frequency, 48000, data_.data() + current_i) / static_cast<double>(SHRT_MAX);
				
				if (dft > threshold) {
					starting_tones_.push_back(current_i);
					current_i += 60000;
					
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

size_t Recording::getTonePlayingWhen(int id) {
	if (static_cast<unsigned int>(id) >= starting_tones_.size())
		ERROR("not all tones were recorded, aborting");
		
	return starting_tones_.at(id);
}

int Recording::getId() {
	return id_;
}

string Recording::getIP() {
	return ip_;
}

string Recording::getLastIP() {
	istringstream stream(ip_);
	vector<string> tokens;
	string token;
	
	while (getline(stream, token, '.'))
		if (!token.empty())
			tokens.push_back(token);
	
	return tokens.back();
}