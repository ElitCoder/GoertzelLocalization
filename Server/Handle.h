#ifndef HANDLE_H
#define HANDLE_H

#include <vector>
#include <string>

class Handle {
public:
	static std::vector<std::vector<double>> handleGetSpeakerVolumeAndCapture(std::vector<std::string>& ips);
};

#endif