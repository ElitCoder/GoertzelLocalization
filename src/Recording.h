#ifndef RECORDING_H
#define RECORDING_H

#include <string>
#include <vector>

class Recording {
public:
	Recording(const std::string& ip);
	
	std::vector<short>& getData();
	void addDistance(int id, double distance);
	double getDistance(int id);
	void setPosition(const std::pair<double, double>& position);
	void findStartingTones(int num_recordings, const int N, double threshold, double reducing, int frequency);
	size_t getTonePlayingWhen(int id) const;
	int getId() const;
	std::string getLastIP() const;
	std::string getIP();
	
private:
	std::string ip_;
	int id_;
	std::vector<short> data_;
	std::vector<size_t> starting_tones_;
	std::vector<std::pair<int, double>> distances_;
	std::pair<double, double> position_;
};

#endif