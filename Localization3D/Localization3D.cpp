#include <iostream>
#include <vector>
#include <cassert>
#include <fstream>
#include <cmath>
#include <set>

#define NUM_DIMENSIONS	(3)

#define ERROR(...)	do { fprintf(stderr, "Error: "); fprintf(stderr, __VA_ARGS__); fprintf(stderr, "\n"); exit(1); } while(0)

using namespace std;

static double g_maxDistance;
static double g_distanceAccuracy = 0.4;
static double g_pointAccuracy = 0.05;

static double PI;

double RelDiff(double a, double b) {
	return abs(a - b);
}

class Point {
public:
	explicit Point(const string& ip, const vector<double>& distances) {
		distances_.insert(distances_.end(), distances.begin(), distances.end());	
		ip_ = ip;
		set = false;
	}
	
	explicit Point(const vector<double>& coordinates) {
		coordinates_.insert(coordinates_.end(), coordinates.begin(), coordinates.end());
		ip_ = "not set";
		set = true;
	}
	
	const string& getIP() {
		return ip_;
	}
	
	void setCoordinates(const vector<double>& coordinates) {
		coordinates_ = coordinates;
	}
	
	const vector<double>& getCoordinates() const {
		assert(coordinates_.size() >= NUM_DIMENSIONS);
		
		return coordinates_;
	}
	
	bool isPlacementSet() {
		return set;
	}
	
	void setPlacement(bool status) {
		set = status;
	}
	
	double getDistanceTo(size_t element) const {
		return distances_.at(element);
	}
	
	bool operator==(const Point& point) {
		for (size_t i = 0; i < coordinates_.size(); i++)
			if (RelDiff(coordinates_.at(i), point.coordinates_.at(i)) > 0.001)
				return false;
				
		return true;
	}
	
	friend ostream& operator<<(ostream& out, const Point& point) {
		cout << "(";
		
		for (size_t i = 0; i < point.coordinates_.size(); i++) {
			cout << point.coordinates_.at(i);
			
			if ((i + 1) != point.coordinates_.size())
				cout << ", ";	
		}
		
		cout << ")";
		
		return out;
	}
	
	friend bool operator<(const Point& a, const Point& b) {
		for (size_t i = 0; i < a.coordinates_.size(); i++)
			if (RelDiff(a.coordinates_.at(i), b.coordinates_.at(i)) >= 0.001)
				return a.coordinates_.at(i) < b.coordinates_.at(i);
				
		// If they are the same point		
		return true;
	}
	
private:
	vector<double> coordinates_;
	vector<double> distances_;
	string ip_;
	bool set;
};

double distanceBetweenPoints(const Point& from, const Point& to) {
	const vector<double>& from_coordinates = from.getCoordinates();
	const vector<double>& to_coordinates = to.getCoordinates();
	
	double euclidian = 0.0;
	
	for (size_t i = 0; i < NUM_DIMENSIONS; i++) {
		double d1 = from_coordinates.at(i);
		double d2 = to_coordinates.at(i);
		
		euclidian += (d2 - d1) * (d2 - d1);
	}
	
	//cout << "Debug: calculated distance to " << sqrt(euclidian) << endl;
	
	return euclidian;
}

bool isDistanceGloballyAccepted(const Point& from, const Point& to, double distance) {
	double calculated_distance = distanceBetweenPoints(from, to);
	distance *= distance;
	double actual_difference = abs(calculated_distance - distance);
	
	return actual_difference < g_distanceAccuracy;
}

double getRadians(double degrees) {
	return (degrees * PI) / 180.0;
}

// TODO: Improve this a lot by using angles and possible points on the sphere instead of brute-force
vector<Point> getPointsOnCircle(const Point& point, double distance) {
	vector<Point> possible_placements;
	int num_iterations = g_maxDistance / g_pointAccuracy;
	
	distance *= distance;
	
	#pragma omp parallel
	{
		vector<Point> temp_placements;
		
		#pragma omp for
		for (int i = -num_iterations; i < num_iterations; i++) {
			double x = i * g_pointAccuracy;
			
			for (int j = -num_iterations; j < num_iterations; j++) {
				double y = j * g_pointAccuracy;
				
				for (int k = -num_iterations; k < num_iterations; k++) {
					double z = k * g_pointAccuracy;
					
					double calculated_distance = distanceBetweenPoints(point, Point({ x, y, z }));
					double actual_difference = abs(calculated_distance - distance);
					
					if (actual_difference < g_distanceAccuracy)
						temp_placements.push_back(Point({ x, y, z }));
				}
			}
		}
		
		#pragma omp critical
		{
			possible_placements.insert(possible_placements.end(), temp_placements.begin(), temp_placements.end());
		}
	}
	
	//cout << "Debug: found " << possible_placements.size() << " placements on the circle\n";		
	
	return possible_placements;			
}

template<class T>
void removeDuplicates(vector<T>& data) {
	set<T> s;
	size_t size = data.size();
	
	for (size_t i = 0; i < size; i++)
		s.insert(data[i]);
		
	data.assign(s.begin(), s.end());
}

vector<Point> getPossiblePlacements(const vector<Point>& constraints, size_t current) {
	vector<Point> possible_placements;
	vector<Point> actually_possible;
	
	for (auto& point : constraints) {
		double distance = point.getDistanceTo(current);
		vector<Point> possibilites = getPointsOnCircle(point, distance);
		
		possible_placements.insert(possible_placements.end(), possibilites.begin(), possibilites.end());
	}
	
	removeDuplicates(possible_placements);
	
	for (auto& point : possible_placements) {
		bool working = true;
		
		for (auto& constraint : constraints) {
			double distance = constraint.getDistanceTo(current);
			
			if (!isDistanceGloballyAccepted(point, constraint, distance)) {
				working = false;

				break;
			}
		}
		
		if (working)
			actually_possible.push_back(point);
	}
	
	return actually_possible;
}

vector<Point> solvePlacement(const vector<Point>& points, size_t current) {
	//cout << "Debug: testing speaker " << current + 1 << " of " << points.size() << endl;
	
	vector<Point> constraints(points.begin(), points.begin() + current);
	vector<Point> possible_placements = getPossiblePlacements(constraints, current);
	
	if (possible_placements.empty()) {
		//cout << "Debug: no possible placements\n";
		
		return vector<Point>();
	}
		
	if (points.size() == current + 1) {
		//cout << "Debug: found possible placement!\n";
		
		vector<Point> correct_placement(points);
		
		correct_placement.at(current).setCoordinates(possible_placements.front().getCoordinates());
		correct_placement.at(current).setPlacement(true);
		
		return correct_placement;
	}
	
	for (size_t i = 0; i < possible_placements.size(); i++) {
		Point& point = possible_placements.at(i);
		
		/*
		if (base) {
			cout << "Trying possiblity " << i + 1 << " of " << possible_placements.size() << endl;
			
			int percentage = static_cast<int>(((i + 1) / static_cast<double>(possible_placements.size())) * 100.0);
			
			if (percentage != g_percentageDone) {
				g_percentageDone = percentage;
				
				cout << percentage << " %\n";
			}
		}
		*/
		
		//cout << "Debug: trying placement " << point << endl;
		
		vector<Point> test_placement(points);
		Point& test_point = test_placement.at(current);
		
		test_point.setCoordinates(point.getCoordinates());
		test_point.setPlacement(true);
		
		vector<Point> result_placement = solvePlacement(test_placement, current + 1);
		
		if (!result_placement.empty())
			return result_placement;
	}
	
	//cout << "Debug: this path did not find any working solutions\n";
	
	return vector<Point>();
}

void parseInput(vector<Point>& points, const string& filename) {
	ifstream file(filename);
	
	if (!file.is_open())
		ERROR("could not open file %s\n", filename.c_str());
		
	string tmp;
	getline(file, tmp);
	
	int num_points = stoi(tmp);
	
	for (int i = 0; i < num_points; i++) {
		string ip;
		getline(file, ip);
		
		vector<double> distances;
		
		for (int j = 0; j < num_points; j++) {
			getline(file, tmp);
			distances.push_back(stod(tmp));
		}
		
		points.push_back(Point(ip, distances));
	}
	
	file.close();
}

void setMaxDistance(const vector<Point>& points) {
	const Point& first = points.front();
	
	for (size_t i = 0; i < points.size(); i++)
		if (first.getDistanceTo(i) > g_maxDistance)
			g_maxDistance = first.getDistanceTo(i);
			
	g_maxDistance *= 1.2;		
}

void calculatePI() {
	PI = atan(1) * 4;
}

int main() {
	vector<Point> points;
	parseInput(points, "live_localization.txt");
	setMaxDistance(points);
	calculatePI();
	
	points.front().setCoordinates({ 0.0, 0.0, 0.0 });
	points.front().setPlacement(true);
	
	vector<Point> placements = solvePlacement(points, 1 /* first speaker has 0, 0, 0 */);
	
	for (auto& point : placements)
		cout << point.getIP() << " " << point << endl;
	
	return 0;
}