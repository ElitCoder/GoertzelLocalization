#ifndef RECORDING_H
#define RECORDING_H

#include <string>
#include <vector>

class Recording {
public:
	Recording(const std::string& filename);
	
	const std::string& getFilename();
	void setStartingPoint(int start);
	int getStartingPoint();
	std::vector<short>& getData();
	void addDistance(int id, double distance);
	double getDistance(int id);
	void setPosition(const std::pair<double, double>& position);
	size_t findActualStart(const int N, double threshold, double reducing, int frequency);
	
private:
	void normalizeActualStart();
	
	std::string filename_;
	int start_;
	std::vector<short> data_;
	std::vector<std::pair<int, double>> distances_;
	std::pair<double, double> position_;
	size_t actual_start_;
};

#endif