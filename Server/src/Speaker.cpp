#include "Speaker.h"

#include <algorithm>
#include <iostream>
#include <cmath>

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

template<class T>
static T getMean(const vector<T>& container) {
	double sum = 0;
	
	for(const auto& element : container)
		sum += element;
		
	cout << "sum " << sum << endl;
	cout << "sum divided " << sum / container.size() << endl;
	cout << "container size " << container.size() << endl;
		
	return lround(sum / container.size());	
}

static void correctMaxEQ(vector<int>& eq) {
	int mean_db = getMean(eq);
	
	for (auto& setting : eq)
		setting -= mean_db;
		
	cout << "Lower setting with " << mean_db << endl;
	
	// See if > max and < min
	int min = *min_element(eq.begin(), eq.end());
	
	if (min > DSP_MIN_EQ) {
		cout << "min lower is " << min << endl;

		// Move everything lower to fit curve
		int delta = min - DSP_MIN_EQ;
		
		cout << "delta is " << delta << endl;
		
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

double Speaker::getBestScore() const {
	return score_;
}

vector<int> Speaker::getBestEQ() {
	if (current_best_eq.empty())
		current_best_eq = vector<int>(9, 0);
		
	return current_best_eq;
}

// Returns current EQ
vector<int> Speaker::setCorrectionEQ(const vector<int>& eq, double score) {
	// Only update best EQ if the score is better
		
	if (correction_eq_.empty())
		correction_eq_ = vector<int>(9, 0);
		
	if (score > score_) {
		current_best_eq = correction_eq_;
		score_ = score;
	}
	
	printEQ(ip_, correction_eq_, "old correction");
	
	if (!unlimited_eq_.empty()) {
		for (size_t i = 0; i < eq.size(); i++)
			unlimited_eq_.at(i) += eq.at(i);
	} else {
		unlimited_eq_ = eq;
	}
	
	if (!correction_eq_.empty()) {
		cout << "Correction EQ was not empty, adding new EQ on top\n";
		
		for (size_t i = 0; i < eq.size(); i++)
			correction_eq_.at(i) += eq.at(i);
	} else {
		correction_eq_ = eq;
	}
	
	printEQ(ip_, correction_eq_, "new correction");
	
	std::vector<int> actual_eq = correction_eq_; //unlimited_eq_;
	correctMaxEQ(actual_eq);
	
	printEQ(ip_, eq, "desired adding");
	printEQ(ip_, unlimited_eq_, "unlimited");
	printEQ(ip_, actual_eq, "actual");
	
	correction_eq_ = actual_eq;
	
	printEQ(ip_, correction_eq_, "final correction");
	
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