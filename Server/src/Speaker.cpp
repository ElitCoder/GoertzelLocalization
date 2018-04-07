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

#if 0
void Speaker::setLinearGainFrom(const string& ip, double db) {
	auto iterator = find_if(mic_gain_responses_.begin(), mic_gain_responses_.end(), [&ip] (auto& peer) {
		return peer.first == ip;
	});
	
	if (iterator == mic_gain_responses_.end())
		mic_gain_responses_.push_back({ ip, db });
	else
		(*iterator) = { ip, db };
		
	cout << "Linear gain from: " << ip << " is " << db << endl;
	//for_each(dbs.begin(), dbs.end(), [] (auto& value) { cout << value << " "; });
	//cout << endl;
}

double Speaker::getLinearGainFrom(const string& ip) const {
	auto iterator = find_if(mic_gain_responses_.begin(), mic_gain_responses_.end(), [&ip] (auto& peer) {
		return peer.first == ip;
	});
	
	if (iterator == mic_gain_responses_.end())
		return -100;
	else
		return iterator->second;
}
#endif

void Speaker::setMicBoost(int boost) {
	mic_boost_ = boost;
	
	cout << "Setting (" << ip_ << ") mic boost to " << mic_boost_ << endl;
}

template<class T>
vector<int> getIntVector(const vector<T>& container) {
	vector<int> result;
	result.reserve(container.size());
	
	for_each(container.begin(), container.end(), [&result] (auto& value) { result.push_back(lround(value)); });
	
	return result;
}

vector<int> Speaker::getCorrectionEQ() {
	if (correction_eq_.empty()) {
		cout << "Server asking for empty correction EQ! Setting it to flat\n";
		
		correction_eq_ = vector<double>(9, 0);
	}
	
	return getIntVector(correction_eq_);
}

template<class T>
static T getMean(const vector<T>& container) {
	double sum = 0;
	
	for(const auto& element : container)
		sum += element;
		
	return sum / container.size();
}

static int correctMaxEQ(vector<double>& eq) {
	double mean_db = getMean(eq);
	
	for (auto& setting : eq)
		setting -= mean_db;
		
	//cout << "Lower setting with " << mean_db << endl;
	
	// See if > max and < min
	/*
	int min = *min_element(eq.begin(), eq.end());
	
	if (min > DSP_MIN_EQ) {
		//cout << "min lower is " << min << endl;

		// Move everything lower to fit curve
		int delta = min - DSP_MIN_EQ;
		
		//cout << "delta is " << delta << endl;
		
		for (auto& setting : eq)
		setting -= delta;
	
	}
	*/
	
	for (auto& setting : eq) {
		if (setting < DSP_MIN_EQ)
			setting = DSP_MIN_EQ;
		else if (setting > DSP_MAX_EQ)
			setting = DSP_MAX_EQ;
	}
	
	return lround(mean_db);
}

static void printEQ(const string& ip, const vector<double>& eq, const string& name) {
	cout << "Setting (" << ip << ") " << name << " EQ to\t";
	
	for (auto setting : eq)
		cout << setting << ", ";
		
	cout << "\n";
}

#if 0
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
#endif

double Speaker::getBestScore() const {
	return score_;
}

vector<int> Speaker::getBestEQ() {
	if (current_best_eq_.empty())
		current_best_eq_ = vector<double>(9, 0);
		
	return getIntVector(current_best_eq_);
}

void Speaker::clearAllEQs() {
	correction_eq_ = vector<double>(9, 0);
	current_best_eq_ = vector<double>(9, 0);
	correction_volume_ = volume_;
	score_ = 0;
	best_speaker_volume_ = volume_;
}

int Speaker::getBestVolume() const {
	return best_speaker_volume_;
}

void Speaker::setBestVolume() {
	volume_ = best_speaker_volume_;
}

void Speaker::setCorrectionVolume() {
	volume_ = correction_volume_;
}

// Returns current EQ
void Speaker::setCorrectionEQ(const vector<double>& eq, double score) {
	printEQ(ip_, eq, "input");
		
	if (correction_eq_.empty())
		correction_eq_ = vector<double>(9, 0);
	
	// Only update best EQ if the score is better
	if (score > score_) {
		current_best_eq_ = correction_eq_;
		score_ = score;
		best_speaker_volume_ = correction_volume_;
	}
	
	for (size_t i = 0; i < eq.size(); i++)
		correction_eq_.at(i) += eq.at(i);
	
	std::vector<double> actual_eq = correction_eq_;
	correction_volume_ = volume_ + correctMaxEQ(actual_eq);
	
	printEQ(ip_, actual_eq, "next");
	
	cout << "Want to change volume to: " << correction_volume_ << endl;
	
	correction_eq_ = actual_eq;
}

int Speaker::getCurrentVolume() const {
	return volume_ > 57 ? 57 : volume_;
}

/*
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
*/

void Speaker::setFrequencyResponseFrom(const string& ip, const vector<double>& dbs) {
	auto iterator = find_if(mic_frequency_responses_.begin(), mic_frequency_responses_.end(), [&ip] (auto& peer) {
		return peer.first == ip;
	});
	
	if (iterator == mic_frequency_responses_.end())
		mic_frequency_responses_.push_back({ ip, dbs });
	else
		(*iterator) = { ip, dbs };
		
	cout << "Frequency response from: " << ip << " is ";
	for_each(dbs.begin(), dbs.end(), [] (auto& value) { cout << value << " "; });
	cout << endl;
}

vector<double> Speaker::getFrequencyResponseFrom(const string& ip) const {
	auto iterator = find_if(mic_frequency_responses_.begin(), mic_frequency_responses_.end(), [&ip] (auto& peer) {
		return peer.first == ip;
	});
	
	if (iterator == mic_frequency_responses_.end())
		return vector<double>();
	else
		return iterator->second;
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