#ifndef HANDLE_H
#define HANDLE_H

#include "SpeakerPlacement.h"

#include <vector>
#include <string>

using SSHOutput = std::vector<std::pair<std::string, std::vector<std::string>>>;
using SpeakerdBs = std::vector<std::pair<std::string, std::vector<std::pair<std::string, double>>>>;
using OwnSoundLevelOutput = std::vector<std::tuple<std::string, int, double>>;

class Handle {
public:
	static SSHOutput handleGetSpeakerVolumeAndCapture(const std::vector<std::string>& ips);
	static SSHOutput handleSetSpeakerVolumeAndCapture(const std::vector<std::string>& ips, const std::vector<double>& volumes, const std::vector<double>& captures, const std::vector<int>& boosts);
	static std::vector<SpeakerPlacement> handleRunLocalization(const std::vector<std::string>& ips, bool skip_script = false);
	static SpeakerdBs handleTestSpeakerdBs(const std::vector<std::string>& ips, int play_time, int idle_time, int num_external, bool skip_script = false);
	static std::vector<bool> checkSpeakerOnline(const std::vector<std::string>& ips);
	static std::vector<std::pair<std::string, double>> checkCurrentSoundImage(const std::vector<std::string>& play_ips, const std::vector<std::string>& listen_ips);
	static OwnSoundLevelOutput checkSpeakerOwnSoundLevel(const std::vector<std::string>& ips);
};

#endif