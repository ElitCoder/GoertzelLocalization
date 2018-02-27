#include "Recording.h"

#include <iostream>

using namespace std;

Recording::Recording(const string& filename, int start) :
	filename_(filename), start_(start) {
}

const string& Recording::getFilename() {
	return filename_;
}

int Recording::getStartingPoint() {
	return start_;
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