#ifndef RECORDING_H
#define RECORDING_H

#include <string>
#include <vector>

enum {
	FIRST,
	SECOND
};

class Recording {
public:
	Recording(const std::string& ip);
	
	std::vector<short>& getData();
	void addDistance(int id, double distance);
	void setDistance(int id, double distance);
	double getDistance(int id);
	void setPosition(const std::pair<double, double>& position);
	void findStartingTones(int num_recordings, const int N, double threshold, double reducing, int frequency);
	void findStartingTonesAmplitude(int num_recordings);
	size_t getTonePlayingWhen(int id) const;
	int getId() const;
	std::string getLastIP() const;
	std::string getIP();
	void setFrameDistance(int id, int which, long long distance);
	long long getFrameDistance(int id, int which);
	
private:
	short getNoiseLevel(int num_recordings);
	
	std::string ip_;
	int id_;
	std::vector<short> data_;
	std::vector<size_t> starting_tones_;
	std::vector<std::pair<int, double>> distances_;
	std::vector<std::pair<int, double>> distances_first_;
	std::vector<std::pair<int, double>> distances_second_;
	std::pair<double, double> position_;
};

#endif