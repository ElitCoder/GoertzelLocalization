#include <iostream>
#include <vector>
#include <cmath>
#include <algorithm>
#include <set>
#include <fstream>

using namespace std;

static double g_distanceAccuracy = 10;
static double PI;

double RelDif(double a, double b) {
	return abs(a - b);
}

class Point {
public:
	explicit Point(const string& ip) {
		set = false;
		ip_ = ip;
	}
	
	explicit Point(double x, double y) :
		x_(x), y_(y) {
		ip_ = "not set";
		set = true;
	}
	
	void setPosition(pair<double, double> position) {
		x_ = position.first;
		y_ = position.second;
		
		set = true;
	}
	
	const string& getIP() {
		return ip_;
	}
	
	pair<double, double> getFinalPosition() {
		return { x_, y_ };
	}
	
	double getX() {
		return x_;
	}
	
	double getY() {
		return y_;
	}
	
	bool isSet() {
		return set;
	}
	
	void addDistance(double distance) {
		distance_.push_back(distance);	
	}
	
	double getDistance(size_t i) {
		return distance_.at(i);
	}
	
	bool operator==(const Point& point) {
		if (RelDif(point.x_, x_) <= 0.001 && RelDif(point.y_, y_) <= 0.001)
			return true;
			
		return false;	
	}
	
	friend ostream& operator<<(ostream& out, const Point& point) {
		cout << "(" << point.x_ << ", " << point.y_ << ")";
		
		return out;
	}
	
	friend bool operator<(const Point& a, const Point& b) {
		if (RelDif(a.x_, b.x_) <= 0.001)
			return a.y_ < b.y_;
			
		return a.x_ < b.x_;	
	}
	
	vector<double> distance_;
	bool set;
	double x_;
	double y_;
	string ip_;
};

namespace PrivateOperations {
	double distanceBetweenPoints(double x1, double y1, double x2, double y2) {
		return (x2 - x1) * (x2 - x1) + (y2 - y1) * (y2 - y1);
	}
}

double distanceBetweenPointsSqrt(double x1, double y1, double x2, double y2) {
	return sqrt(PrivateOperations::distanceBetweenPoints(x1, y1, x2, y2));
}

double toRadians(double degrees) {
	return (degrees * PI) / 180;
}

vector<Point> getSinglePossibles(Point& point, double actual_distance) {
	vector<Point> possibles;
	
	for (int a = 0; a < 360; a++) {
		double x = point.getX() + actual_distance * cos(toRadians(a));
		double y = point.getY() + actual_distance * sin(toRadians(a));

		possibles.push_back(Point(x, y));;
	}
	
	return possibles;
}

template<class T>
void removeDuplicates(vector<T>& data) {
	set<T> s;
	size_t size = data.size();
	for( size_t i = 0; i < size; ++i ) s.insert( data[i] );
	data.assign( s.begin(), s.end() );
}

vector<Point> getPossibles(vector<Point>& points, size_t i) {
	vector<Point> possibles;
	vector<Point> working;
	
	#pragma omp parallel
	{
		vector<Point> parallel_possibles;
		
		#pragma omp for
		for (size_t j = 0; j < points.size(); j++) {
			Point& master = points.at(j);
			double distance = master.getDistance(i);
			auto master_possibles = getSinglePossibles(master, distance);
			
			parallel_possibles.insert(parallel_possibles.end(), master_possibles.begin(), master_possibles.end());
		}
		
		#pragma omp critical
		{
			possibles.insert(possibles.end(), parallel_possibles.begin(), parallel_possibles.end());
		}
	}
	
	removeDuplicates(possibles);
	
	#pragma omp parallel
	{
		vector<Point> parallel_working;
		
		#pragma omp for
		for (size_t j = 0; j < possibles.size(); j++) {
			Point& point = possibles.at(j);
			bool good = true;
			
			for (auto& origin : points) {
				double distance = origin.getDistance(i);
				double test_distance = distanceBetweenPointsSqrt(point.getX(), point.getY(), origin.getX(), origin.getY());
				//distance *= distance;
				//if (abs(distance - distanceBetweenPointsNoSqrt(point.getX(), point.getY(), origin.getX(), origin.getY())) > g_distanceAccuracy) {
				if (abs(distance - test_distance) > g_distanceAccuracy) {
					good = false;
					
					break;
				}
			}

			if (good) {
				parallel_working.push_back(point);
			}
		}
		
		#pragma omp critical
		{
			working.insert(working.end(), parallel_working.begin(), parallel_working.end());
		}
	}
	
	return working;
}

vector<Point> getPlacement(vector<Point> points, size_t start) {
	if (start >= points.size())
		return points;
		
	vector<Point> origins(points.begin(), points.begin() + start);
	vector<Point> possibles = getPossibles(origins, start);
	
	// Do we have any possibility?
	if (possibles.empty()) {
		//cout << "gave no possibles\n";
		
		return vector<Point>();
	}
	
	// We do, let's see if this is the last point
	if (points.size() == start + 1) {
		points.at(start).setPosition(possibles.front().getFinalPosition());
		//cout << "point: " << points.at(start) << endl;
		//cout << "set position of end condition\n";
		
		return points;
	}
	
	// It's not the last point, let's try every possiblity
	for (auto& possible : possibles) {
		points.at(start).setPosition(possible.getFinalPosition());
		//cout << "testing position\n";
		
		vector<Point> result = getPlacement(points, start + 1);
		
		if (!result.empty()) {
			//cout << "found working\n";
			
			return result;
		}
	}
	
	//cout << "SHOULD NOT BE HERE\n";
	
	return vector<Point>();
}

void printHelp() {
	cout << "Usage: ./Localization <starting distance accuracy>\n";
	cout << "Print this message with -h or --help\n";
	cout << "\nReads input_structure from stdin and calculates distance between points, with accuracy starting at <starting distance accuracy> (default 0.6m)\n";
}

void writeResultsToServer(vector<Point>& results) {
	ofstream file("server_placements_results.txt");
	
	if (!file.is_open()) {
		cout << "Warning: could not open file for writing results to server\n";
		
		return;
	}
	
	for (auto& point : results) {
		file << point.getIP() << " " << point.getX() << " " << point.getY() << endl;
	}
	
	file.close();
}

int main(int argc, char** argv) {
	if (argc == 2) {
		if (string(argv[1]) == "-h" || string(argv[1]) == "--help") {
			printHelp();
			
			return 0;
		}
	}
	
	if (argc >= 2) {
		g_distanceAccuracy = stod(argv[1]);
		
		cout << "Setting distance accuracy to " << g_distanceAccuracy << endl;
	}
	
	PI = atan(1) * 4;
	cout << "Setting PI to " << PI << endl;
	
	string tmp;
	getline(cin, tmp);
	
	size_t num_points;
	num_points = stoi(tmp);
	
	vector<string> ips;
	
	for (size_t i = 0; i < num_points; i++) {
		string ip;
		getline(cin, ip);
				
		ips.push_back(ip);
	}
	
	vector<Point> points;
	
	for (size_t i = 0; i < num_points; i++) {
		Point point(ips.at(i));
		
		for (size_t j = 0; j < num_points; j++) {
			double distance;
			cin >> distance;
			
			point.addDistance(distance);
			
			printf("%zu -> %zu\t= %1.2f\n", i, j, distance);
		}
		
		points.push_back(point);
	}
	
	points.front().setPosition({ 0, 0 });
	
	while (g_distanceAccuracy > 0) {
		cout << "Trying for accuracy " << g_distanceAccuracy << " m\n\n";
		
		vector<Point> basic_points(points);
		auto results = getPlacement(basic_points, 1);
		
		if (results.empty()) {
			cout << "No more possible results\n";
			
			break;
		}
		
		for (auto& point : results) {
			if (!point.isSet())
				break;
				
			auto position = point.getFinalPosition();
			
			printf("%s: (%1.2f, %1.2f)\n", point.getIP().c_str(), position.first, position.second);
		}
		
		cout << endl;
		
		for (auto& point : results) {
			if (!point.isSet())
				break;
				
			auto position = point.getFinalPosition();
			
			printf("(%1.2f, %1.2f)\n", position.first, position.second);	
		}
		
		cout << endl;
		
		writeResultsToServer(results);
		
		g_distanceAccuracy -= 0.01;
	}
	
	return 0;
}