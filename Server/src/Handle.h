#ifndef HANDLE_H
#define HANDLE_H

#include "SpeakerPlacement.h"

#include <vector>
#include <string>

using SpeakerdBs = std::vector<std::pair<std::string, std::vector<std::pair<std::string, double>>>>;
using SSHOutput = std::vector<std::pair<std::string, std::vector<std::string>>>;
using SoundImageFFT9 = std::vector<std::pair<std::string, std::vector<double>>>;

class Handle {
public:
	static bool handleSetSpeakerVolumeAndCapture(const std::vector<std::string>& ips, const std::vector<int>& volumes, const std::vector<int>& captures, const std::vector<int>& boosts);
	static std::vector<SpeakerPlacement> handleRunLocalization(const std::vector<std::string>& ips, bool skip_script = false);
	static SpeakerdBs handleTestSpeakerdBs(const std::vector<std::string>& ips, int play_time, int idle_time, int num_external, bool skip_script = false, bool do_normalize = true);
	static SpeakerdBs handleTestSpeakerdBs(const std::vector<std::string>& speakers, const std::vector<std::string>& mics, int play_time, int idle_time);
	static std::vector<bool> checkSpeakerOnline(const std::vector<std::string>& ips);
	static SoundImageFFT9 handleSoundImage(const std::vector<std::string>& speakers, const std::vector<std::string>& mics, int play_time, int idle_time);
};

#endif