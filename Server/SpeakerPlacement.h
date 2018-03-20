#ifndef SPEAKER_PLACEMENT_H
#define SPEAKER_PLACEMENT_H

#include <array>
#include <vector>

enum {
	NUM_DIMENSIONS = 2
};

class SpeakerPlacement {
public:
	explicit SpeakerPlacement(const std::string& ip);
	
	void setCoordinates(const std::array<double, NUM_DIMENSIONS>& coordinates);
	void addDistance(const std::string& ip, double distance);
	
	const std::array<double, NUM_DIMENSIONS>& getCoordinates();
	const std::vector<std::pair<std::string, double>>& getDistances();
	const std::string& getIp();
	
private:
	std::vector<std::pair<std::string, double>> distances_;
	std::array<double, NUM_DIMENSIONS> coordinates_;
	
	std::string ip_;
};

#endif