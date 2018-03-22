#include "Recording.h"

#include <iostream>
#include <sstream>
#include <climits>
#include <algorithm>
#include <cmath>

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

// Taken from git/SO
static double goertzel(int numSamples,float TARGET_FREQUENCY,int SAMPLING_RATE, short* data)
{
    int     k,i;
    float   floatnumSamples;
    float   omega,sine,cosine,coeff,q0,q1,q2,magnitude,real,imag;

    float   scalingFactor = numSamples / 2.0;

    floatnumSamples = (float) numSamples;
    k = (int) (0.5 + ((floatnumSamples * TARGET_FREQUENCY) / (float)SAMPLING_RATE));
    omega = (2.0 * M_PI * k) / floatnumSamples;
    sine = sin(omega);
    cosine = cos(omega);
    coeff = 2.0 * cosine;
    q0=0;
    q1=0;
    q2=0;

    for(i=0; i<numSamples; i++)
    {
        q0 = coeff * q1 - q2 + data[i];
        q2 = q1;
        q1 = q0;
    }

    // calculate the real and imaginary results
    // scaling appropriately
    real = (q1 - q2 * cosine) / scalingFactor;
    imag = (q2 * sine) / scalingFactor;

    magnitude = sqrtf(real*real + imag*imag);
    return magnitude;
}

void Recording::findStartingTones(int num_recordings, const int N, double threshold, double reducing, int frequency, int total_play_time_frames, int idle_time) {
	double dft;
	double orig_threshold = threshold;
	size_t current_i = idle_time * 48000 - 0.5 * (double)48000; // Should start at g_playingLength - 0.5 sec
	size_t frame_ending = ((double)idle_time - 0.5) * 48000 + total_play_time_frames;
	
	cout << "Checking first tone at " << current_i / (double)48000 << " to " << frame_ending / (double)48000 << "\n";
	
	for (int i = 0; i < num_recordings; i++) {
		threshold = orig_threshold;
		
		while (threshold > 0) {
			size_t delta_current = current_i;
			bool found = false;
			
			for (; delta_current < frame_ending; delta_current++) {
				dft = goertzel(N, frequency, 48000, data_.data() + delta_current) / static_cast<double>(SHRT_MAX);
				
				if (dft > threshold) {
					starting_tones_.push_back(delta_current);
					//current_i = (total_play_time_frames * (i + 2)) + 0.5 * (double)48000;
					//frame_ending = 48000 + (total_play_time_frames * (i + 2)) + 1.5 * (double)48000;
					current_i = ((double)idle_time - 0.5) * 48000 + total_play_time_frames * (i + 1);
					frame_ending = ((double)idle_time - 0.5) * 48000 + total_play_time_frames * (i + 2);
					
					cout << "delta_current " << delta_current << endl;
					
					cout << "Found tone, checking " << current_i / (double)48000 << " to " << frame_ending / (double)48000 << " now\n";
					
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
	if (static_cast<unsigned int>(id) >= starting_tones_.size()) {
		cout << "Warning: tone detection failed\n";
		
		return 0;
	}
		
	return starting_tones_.at(id);
}

int Recording::getId() const {
	return id_;
}