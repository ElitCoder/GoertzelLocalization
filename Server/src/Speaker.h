#ifndef SPEAKER_H
#define SPEAKER_H

#include <string>
#include <vector>

class Speaker {
public:
	void setIP(const std::string& ip);
	void setEQ(const std::vector<int>& eq);
	void setVolume(int volume);
	void setMicVolume(int volume);
	void setMicBoost(int boost);
	void setOnline(bool status);
	
	const std::string& getIP() const;
	bool isOnline() const;
	
	bool operator==(const std::string& ip);
	
private:
	std::vector<int> eq_;
	
	std::string ip_	= "not set";
	int volume_		= 0;
	int mic_volume_	= 0;
	int mic_boost_	= 0;
	bool online_	= false;
};

#endif