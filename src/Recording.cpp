#include "Recording.h"
#include "Goertzel.h"

#include "matplotlibcpp.h"

#include <iostream>
#include <sstream>
#include <climits>

#define ERROR(...)	do { fprintf(stderr, "Error: "); fprintf(stderr, __VA_ARGS__); fprintf(stderr, "\n"); exit(1); } while(0)

namespace plt = matplotlibcpp;
using namespace std;

template<class T>
static void plot(vector<T>& data) {
	vector<int>	tmp;
	for_each(data.begin(), data.end(), [&tmp] (T& element) { tmp.push_back(element); });
	
	plt::plot(tmp);
	plt::show();
}

Recording::Recording(const string& filename, const string& ip) :
	filename_(filename), ip_(ip) {
	static int id;
	
	id_ = id++;
}

const string& Recording::getFilename() {
	return filename_;
}

int Recording::getStartingPoint() {
	return start_;
}

void Recording::setStartingPoint(int start) {
	start_ = start;
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

/* Needs to be more general later on */
void Recording::normalizeActualStart() {
	actual_start_ /= 1000; // 288000 -> 288
	actual_start_ /= 100; // 288 -> 2.88 -> 2
	actual_start_ *= 1e05; // 2 -> 200000
}

size_t Recording::findActualStart(const int N, double threshold, double reducing, int frequency) {
	double		dft;
			
	//plot(data_);
			
	while (threshold > 0) {
		for (size_t i = 0; i < data_.size(); i += N) {
			dft = goertzel(N, frequency, 48000 /* read this from wav file later */, data_.data() + i) / static_cast<double>(SHRT_MAX);
			
			if (dft > threshold) {
				actual_start_ = i;
				
				//cout << "Actual start for " << filename_ << " is " << actual_start_ << endl;
				normalizeActualStart();
				//cout << "Normalized actual start for " << filename_ << " is " << actual_start_ << endl;
				
				return actual_start_;
			}
		}
		
		threshold -= reducing;
	}
	
	cout << "Warning: could not find actual starting point\n";
	
	return actual_start_;
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

string Recording::getLastIP() {
	istringstream stream(ip_);
	vector<string> tokens;
	string token;
	
	while (getline(stream, token, '.')) {
	    if (!token.empty())
	        tokens.push_back(token);
	}
	
	return "." + tokens.back();
}