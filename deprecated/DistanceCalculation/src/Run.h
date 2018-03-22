#ifndef RUN_H
#define RUN_H

#include <vector>
#include <string>

enum RUN_TYPES {
	RUN_GOERTZEL,
	RUN_NOTHING,
	JUST_RUN_FULL,
	JUST_RUN_SIMPLE,
	RUN_WHITE_NOISE
};

namespace Run {
	void runGoertzel(const std::vector<std::string>& filenames, const std::vector<std::string>& ips);
	void runWhiteNoise(const std::vector<std::string>& filenames, const std::vector<std::string>& ips);
}

#endif 