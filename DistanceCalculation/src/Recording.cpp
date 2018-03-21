#include "Recording.h"
#include "Goertzel.h"

#include <iostream>
#include <sstream>
#include <climits>
#include <algorithm>

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

short Recording::getNoiseLevel(int num_recordings) {
	unsigned long long sum = 0;
	unsigned long long num_frames = 0;
	
	size_t start_noise = 0;
	size_t stop_noise = g_playingLength + 0.5 * (double)48000;
	num_frames = stop_noise - start_noise;
	
	for (size_t i = start_noise; i < stop_noise; i++)
		sum += abs(data_.at(i));
	
	start_noise = static_cast<double>(1 * 48000 + num_recordings * g_playingLength + 1.5 * 48000);
	stop_noise = data_.size();
	num_frames += stop_noise - start_noise;
	
	for (size_t i = start_noise; i < stop_noise; i++)
		sum += abs(data_.at(i));

	return sum / num_frames;
}

static short getAverage(const vector<short>& data, size_t start, size_t end) {
	unsigned long long sum = 0;
	
	for (size_t i = start; i < end; i++)
		sum += abs(data.at(i));
		
	return sum / (end - start);
}

void Recording::findStartingTonesAmplitude(int num_recordings) {
	size_t current_frame = g_playingLength + 0.5 * (double)48000;
	size_t frame_stop = 48000 + (g_playingLength * (0 + 1)) + 1.5 * (double)48000;
	
	long long sum = 0;
	for (auto& element : data_)
		sum += abs(element);
		
	cout << "Debug: total average value: " << sum / data_.size() << endl;
	cout << "Debug: max element: " << *max_element(data_.begin(), data_.end()) << endl;
	
	short noise_level = getNoiseLevel(num_recordings);
	cout << "Debug: noise level = " << noise_level << endl;
	
	double threshold = noise_level * 3;
	double reducing = 2;
	double original_threshold = threshold;
	
	for (int i = 0; i < num_recordings; i++) {
		threshold = original_threshold;
		short window_max = *max_element(data_.begin() + current_frame, data_.begin() + frame_stop);
		
		cout << "Debug: window_max = " << window_max << endl;
		cout << "Debug: finding " << i << " at " << (double)current_frame / 48000 << " to " << (double)frame_stop / 48000 << endl;
		
		while (threshold > 0) {
			size_t starting_frame = current_frame;
			bool found = false;
			
			for (; starting_frame < frame_stop; starting_frame++) {
				//double value = data_.at(starting_frame); // TODO: to some kind of amplitude check here (impulse?)
				
				// Normalize threshold
				//double normalized_threshold = SHRT_MAX / window_max;
				//value *= normalized_threshold;
				
				//if (value > threshold) {
					// Is it possible this is the start? Make sure the average starting here makes sense
					double actual_avg = getAverage(data_, ((double)(current_frame + 0.7 * 48000)), ((double)(current_frame + 0.7 * 48000)) + (48000 / 2));
					double avg = getAverage(data_, starting_frame, starting_frame + 1024);
					
					// We've gone too far
					if (avg > actual_avg)
						continue;
						
					if (avg < actual_avg * 0.7)
						continue;
						
					cout << "Debug: testing " << id_ << " find " << i << " at " << (double)starting_frame / 48000 << " s\n";
					cout << "Debug: found breaking with avg " << avg << " and actual_avg " << actual_avg << endl;	
					
					starting_tones_.push_back(starting_frame);
					
					current_frame = (g_playingLength * (i + 2)) + 0.5 * (double)48000;
					frame_stop = 48000 + (g_playingLength * (i + 2)) + 1.5 * (double)48000;
					
					found = true;
					break;
				//}
			}
			
			if (found)
				break;
				
			threshold -= reducing;
		}
		
		//cout << "Set " << id_ << " find " << i << " at " << (double)starting_tones_.back() / 48000 << " s\n";
		//break;
	}
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