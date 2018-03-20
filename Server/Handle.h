#ifndef HANDLE_H
#define HANDLE_H

#include <vector>
#include <string>

using SSHOutput = std::vector<std::pair<std::string, std::vector<std::string>>>;

class Handle {
public:
	static SSHOutput handleGetSpeakerVolumeAndCapture(const std::vector<std::string>& ips);
	static SSHOutput handleSetSpeakerVolumeAndCapture(const std::vector<std::string>& ips, const std::vector<double>& volumes, const std::vector<double>& captures);
};

#endif