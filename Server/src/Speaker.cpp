#include "Speaker.h"

#include <algorithm>

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