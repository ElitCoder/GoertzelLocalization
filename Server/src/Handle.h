#ifndef HANDLE_H
#define HANDLE_H

#include "SpeakerPlacement.h"

#include <vector>
#include <string>

using SSHOutput = std::vector<std::pair<std::string, std::vector<std::string>>>;
using SpeakerdBs = std::vector<std::vector<std::pair<std::string, double>>>;

class Handle {
public:
	static SSHOutput handleGetSpeakerVolumeAndCapture(const std::vector<std::string>& ips);
	static SSHOutput handleSetSpeakerVolumeAndCapture(const std::vector<std::string>& ips, const std::vector<double>& volumes, const std::vector<double>& captures, const std::vector<int>& boosts);
	static std::vector<SpeakerPlacement> handleRunLocalization(const std::vector<std::string>& ips, int type_localization);
	static SpeakerdBs handleTestSpeakerdBs(const std::vector<std::string>& ips, int play_time, int idle_time, bool skip_script = false);
};

#endif