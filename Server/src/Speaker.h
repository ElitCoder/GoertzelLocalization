#ifndef SPEAKER_H
#define SPEAKER_H

#include <string>
#include <vector>
#include <array>

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
	void setCorrectionEQ(const std::vector<int>& eq);
	void setVolume(int volume);
	void setMicVolume(int volume);
	void setMicBoost(int boost);
	void setOnline(bool status);
	void setPlacement(const SpeakerPlacement& placement, int placement_id);
	void setTargetMeanDB(double mean);
	void setFlatResults(const std::vector<double>& dbs);
	
	const std::string& getIP() const;
	int getPlacementID() const;
	SpeakerPlacement& getPlacement();
	bool isOnline() const;
	bool hasPlacement() const;
	const std::vector<int>& getCorrectionEQ();
	double getTargetMeanDB() const;
	std::vector<double> getFlatResults() const;
	
	bool operator==(const std::string& ip);
	
private:
	// Current EQ
	std::vector<int> eq_;
	double mean_db_	= -1;
	std::vector<double> flat_results_;
	
	// Correction EQ - make it sound better
	std::vector<int> correction_eq_;
	
	std::string ip_	= "not set";
	int volume_		= 0;
	int mic_volume_	= 0;
	int mic_boost_	= 0;
	bool online_	= false;

	SpeakerPlacement placement_;
	int last_placement_id_	= -1;
};

#endif