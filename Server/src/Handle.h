#ifndef HANDLE_H
#define HANDLE_H

#include "SpeakerPlacement.h"

#include <vector>
#include <string>

using SSHOutput = std::vector<std::pair<std::string, std::vector<std::string>>>;
using SoundImageFFT9 = std::vector<std::pair<std::string, std::vector<double>>>;

class Handle {
public:
	static bool setSpeakerAudioSettings(const std::vector<std::string>& ips, const std::vector<int>& volumes, const std::vector<int>& captures, const std::vector<int>& boosts);
	static std::vector<SpeakerPlacement> runLocalization(const std::vector<std::string>& ips, bool skip_script = false);
	static std::vector<bool> checkSpeakersOnline(const std::vector<std::string>& ips);
	static SoundImageFFT9 checkSoundImage(const std::vector<std::string>& speakers, const std::vector<std::string>& mics, int play_time, int idle_time);
	static bool setEQ(const std::vector<std::string>& speakers, const std::vector<double>& settings);
};

#endif