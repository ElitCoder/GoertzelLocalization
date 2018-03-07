#ifndef DELTA_CONTAINER
#define DELTA_CONTAINER

#include <cstddef>
#include <vector>
#include <iostream>

class DeltaContainer {
public:
	DeltaContainer(size_t from, size_t to);
	
	void add(double value);
	double mean();
	
	bool operator==(const DeltaContainer& container);
	
	friend std::ostream& operator<<(std::ostream& out, const DeltaContainer& container);
	
private:
	std::vector<double> deltas_;
	
	size_t from_;
	size_t to_;
};

#endif