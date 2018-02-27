#ifndef RECORDING_H
#define RECORDING_H

#include <string>
#include <vector>

class Recording {
public:
	Recording(const std::string& filename, int start);
	
	const std::string& getFilename();
	int getStartingPoint();
	std::vector<short>& getData();
	void addDistance(int id, double distance);
	double getDistance(int id);
	void setPosition(const std::pair<double, double>& position);
	
private:
	std::string filename_;
	int start_;
	std::vector<short> data_;
	std::vector<std::pair<int, double>> distances_;
	std::pair<double, double> position_;
};

#endif