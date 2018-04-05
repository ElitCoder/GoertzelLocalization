#include "Speaker.h"

#include <algorithm>
#include <iostream>

using namespace std;

const string& Speaker::getIP() const {
	return ip_;
}

void Speaker::setOnline(bool status) {
	online_ = status;
}

bool Speaker::operator==(const string& ip) {
	return ip_ == ip;
}

void Speaker::setIP(const string& ip) {
	ip_ = ip;
}

bool Speaker::isOnline() const {
	return online_;
}

void Speaker::setPlacement(const SpeakerPlacement& placement, int placement_id) {
	placement_ = placement;
	last_placement_id_ = placement_id;
}

bool Speaker::hasPlacement() const {
	return last_placement_id_ >= 0;
}

int Speaker::getPlacementID() const {
	return last_placement_id_;
}

Speaker::SpeakerPlacement& Speaker::getPlacement() {
	return placement_;
}

void Speaker::setVolume(int volume) {
	volume_ = volume;
	
	cout << "Setting (" << ip_ << ") volume to " << volume_ << endl;
}

void Speaker::setMicVolume(int volume) {
	mic_volume_ = volume;
	
	cout << "Setting (" << ip_ << ") mic volume to " << mic_volume_ << endl;
}

void Speaker::setMicBoost(int boost) {
	mic_boost_ = boost;
	
	cout << "Setting (" << ip_ << ") mic boost to " << mic_boost_ << endl;
}

const std::vector<int>& Speaker::getCorrectionEQ() {
	if (correction_eq_.empty()) {
		cout << "Server asking for empty correction EQ! Setting it to flat\n";
		
		correction_eq_ = vector<int>(9, 0);
	}
	
	return correction_eq_;
}

static void correctMaxEQ(vector<int>& eq) {
	// See if > max and < min
	int max = *max_element(eq.begin(), eq.end());
	int min = *min_element(eq.begin(), eq.end());
	
	if (max > DSP_MAX_EQ && min > DSP_MIN_EQ) {
		// Move everything lower to fit curve
		int delta = min - DSP_MIN_EQ;
		
		for (auto& setting : eq)
			setting -= delta;
	} else if (min < DSP_MIN_EQ && max < DSP_MAX_EQ) {
		// Move everything higher to fit curve
		int delta = max - DSP_MAX_EQ;
		
		for (auto& setting : eq)
			setting -= delta;
	}
	
	for (auto& setting : eq) {
		if (setting < DSP_MIN_EQ)
			setting = DSP_MIN_EQ;
		else if (setting > DSP_MAX_EQ)
			setting = DSP_MAX_EQ;
	}
}

static void printEQ(const string& ip, const vector<int>& eq, const string& name) {
	cout << "Setting (" << ip << ") " << name << " EQ to\t";
	
	for (auto setting : eq)
		cout << setting << ", ";
		
	cout << "\n";
}

// Returns reference EQ (flat)
void Speaker::setEQ(const vector<int>& eq) {
	eq_ = eq;
	
	//printEQ(ip_, eq, "flat");
	
	/*
	cout << "Setting (" << ip_ << ") EQ to ";
	
	for (auto setting : eq_)
		cout << setting << ", ";
		
	cout << "\n";
	*/
}

// Returns current EQ
vector<int> Speaker::setCorrectionEQ(const vector<int>& eq) {
	printEQ(ip_, correction_eq_, "old correction");
	
	if (!correction_eq_.empty()) {
		cout << "Correction EQ was not empty, adding new EQ on top\n";
		
		for (size_t i = 0; i < eq.size(); i++)
			correction_eq_.at(i) += eq.at(i);
			
	} else {
		correction_eq_ = eq;
	}
	
	correctMaxEQ(correction_eq_);
	
	printEQ(ip_, eq, "desired adding");
	printEQ(ip_, correction_eq_, "new correction");
	
	return correction_eq_;
}

void Speaker::setTargetMeanDB(double mean) {
	mean_db_ = mean;
}

double Speaker::getTargetMeanDB() const {
	return mean_db_;
}

void Speaker::setFlatResults(const vector<double>& dbs) {
	if (!flat_results_.empty())
		return;
		
	flat_results_ = dbs;	
}

vector<double> Speaker::getFlatResults() const {
	if (flat_results_.empty()) {
		cout << "Server asking for empty flat results, returning 0\n";
		
		return vector<double>(9, 0);
	}
	
	return flat_results_;
}

/*
	SpeakerPlacement
*/

Speaker::SpeakerPlacement::SpeakerPlacement() {}

Speaker::SpeakerPlacement::SpeakerPlacement(const string& ip) {
	ip_ = ip;
}

void Speaker::SpeakerPlacement::setCoordinates(const array<double, 3>& coordinates) {
	coordinates_ = coordinates;
}

void Speaker::SpeakerPlacement::addDistance(const string& ip, double distance) {
	auto iterator = find_if(distances_.begin(), distances_.end(), [&ip] (const pair<string, double>& peer) { return peer.first == ip; });
	
	if (iterator == distances_.end())
		distances_.push_back({ ip, distance });
	else
		(*iterator) = { ip, distance };
}

const array<double, 3>& Speaker::SpeakerPlacement::getCoordinates() {
	return coordinates_;
}

const vector<pair<string, double>>& Speaker::SpeakerPlacement::getDistances() {
	return distances_;
}

const string& Speaker::SpeakerPlacement::getIp() {
	return ip_;
}