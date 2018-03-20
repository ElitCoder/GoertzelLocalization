#include "SpeakerPlacement.h"

#include <algorithm>

using namespace std;

SpeakerPlacement::SpeakerPlacement(const string& ip) {
	ip_ = ip;
}

void SpeakerPlacement::setCoordinates(const array<double, NUM_DIMENSIONS>& coordinates) {
	coordinates_ = coordinates;
}

void SpeakerPlacement::addDistance(const string& ip, double distance) {
	auto iterator = find_if(distances_.begin(), distances_.end(), [&ip] (const pair<string, double>& peer) { return peer.first == ip; });
	
	if (iterator == distances_.end())
		distances_.push_back({ ip, distance });
	else
		(*iterator) = { ip, distance };
}

const array<double, NUM_DIMENSIONS>& SpeakerPlacement::getCoordinates() {
	return coordinates_;
}

const vector<pair<string, double>>& SpeakerPlacement::getDistances() {
	return distances_;
}

const string& SpeakerPlacement::getIp() {
	return ip_;
}