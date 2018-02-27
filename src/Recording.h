#ifndef RECORDING_H
#define RECORDING_H

#include <string>
#include <vector>

class Recording {
public:
	Recording(const std::string& filename, const std::string& ip);
	
	const std::string& getFilename();
	void setStartingPoint(int start);
	int getStartingPoint();
	std::vector<short>& getData();
	void addDistance(int id, double distance);
	double getDistance(int id);
	void setPosition(const std::pair<double, double>& position);
	size_t findActualStart(const int N, double threshold, double reducing, int frequency);
	void findStartingTones(int num_recordings, const int N, double threshold, double reducing, int frequency);
	size_t getTonePlayingWhen(int id);
	int getId();
	std::string getLastIP();
	
private:
	void normalizeActualStart();
	
	std::string filename_;
	std::string ip_;
	int id_;
	int start_;
	std::vector<short> data_;
	std::vector<size_t> starting_tones_;
	std::vector<std::pair<int, double>> distances_;
	std::pair<double, double> position_;
	size_t actual_start_;
};

#endif