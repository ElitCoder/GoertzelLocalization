#include "DeltaContainer.h"

#include <algorithm>

using namespace std;

DeltaContainer::DeltaContainer(size_t from, size_t to) {
	from_ = from;
	to_ = to;
}

void DeltaContainer::add(double value) {
	deltas_.push_back(value);
}

double DeltaContainer::mean() {
	double sum = 0;
	
	for_each(deltas_.begin(), deltas_.end(), [&sum] (double value) { sum += value; });
	
	return sum / deltas_.size();
}

bool DeltaContainer::operator==(const DeltaContainer& container) {
	return container.from_ == from_ && container.to_ == to_;
}

ostream& operator<<(ostream& out, const DeltaContainer& container) {
	cout << "(" << container.from_ << " -> " << container.to_ << "):" << endl;;
	for_each(container.deltas_.begin(), container.deltas_.end(), [&out] (double value) { out << value << endl; });
	cout << endl;
	
	return out;
}