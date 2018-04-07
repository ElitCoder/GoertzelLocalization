#ifndef SPEAKER_H
#define SPEAKER_H

#include <string>
#include <vector>
#include <array>

enum {
	DSP_MAX_EQ = 12,
	DSP_MIN_EQ = -12
};

class Speaker {
public:
	// For placing speakers
	class SpeakerPlacement {
	public:
		explicit SpeakerPlacement();
		explicit SpeakerPlacement(const std::string& ip);
		
		void setCoordinates(const std::array<double, 3>& coordinates);
		void addDistance(const std::string& ip, double distance);
		
		const std::array<double, 3>& getCoordinates();
		const std::vector<std::pair<std::string, double>>& getDistances();
		const std::string& getIp();
		
	private:
		std::vector<std::pair<std::string, double>> distances_;
		std::array<double, 3> coordinates_;
		
		std::string ip_;
	};
	
	void setIP(const std::string& ip);
	void setEQ(const std::vector<int>& eq);
	void setCorrectionEQ(const std::vector<double>& eq, double score);
	void setVolume(int volume);
	void setMicVolume(int volume);
	void setMicBoost(int boost);
	void setOnline(bool status);
	void setPlacement(const SpeakerPlacement& placement, int placement_id);
	void setFrequencyResponseFrom(const std::string& ip, const std::vector<double>& dbs);
	void clearAllEQs();
	void setLinearGainFrom(const std::string& ip, double db);
	void setBestVolume();
	void setCorrectionVolume();
	
	const std::string& getIP() const;
	int getPlacementID() const;
	SpeakerPlacement& getPlacement();
	bool isOnline() const;
	bool hasPlacement() const;
	std::vector<int> getCorrectionEQ();
	std::vector<int> getBestEQ();
	int getBestVolume() const;
	double getBestScore() const;
	std::vector<double> getFrequencyResponseFrom(const std::string& ip) const;
	double getLinearGainFrom(const std::string& ip) const;
	int getCurrentVolume() const;

	bool operator==(const std::string& ip);
	
private:
	// Current flat EQ
	std::vector<int> eq_;
	
	// Correction EQ - make it sound better
	std::vector<double> correction_eq_;
	std::vector<double> current_best_eq_;
	double score_ = 0.0;
	int best_speaker_volume_ = 0;
	int correction_volume_ = 0;
	
	// Information about frequency response from other speakers
	std::vector<std::pair<std::string, std::vector<double>>> mic_frequency_responses_;
	std::vector<std::pair<std::string, double>> mic_gain_responses_; 
	
	std::string ip_	= "not set";
	int volume_		= 0;
	int mic_volume_	= 0;
	int mic_boost_	= 0;
	bool online_	= false;

	SpeakerPlacement placement_;
	int last_placement_id_	= -1;
};

#endif