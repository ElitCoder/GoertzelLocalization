#include "Recording.h"
#include "Goertzel.h"

#include "matplotlibcpp.h"

#include <iostream>
#include <climits>

namespace plt = matplotlibcpp;
using namespace std;

template<class T>
static void plot(vector<T>& data) {
	vector<int>	tmp;
	for_each(data.begin(), data.end(), [&tmp] (T& element) { tmp.push_back(element); });
	
	plt::plot(tmp);
	plt::show();
}

Recording::Recording(const string& filename) :
	filename_(filename) {
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