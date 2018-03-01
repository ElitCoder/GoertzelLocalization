#include <iostream>
#include <vector>
#include <cmath>
#include <algorithm>
#include <set>

#define DISTANCE_ACCURACY	(1)
#define POINT_ACCURACY		(0.1)

using namespace std;

static double g_maxDistance;

double RelDif(double a, double b) {
	return abs(a - b);
}

class Point {
public:
	explicit Point() {
		set = false;
	}
	
	explicit Point(double x, double y) :
		x_(x), y_(y) {
		set = true;
	}
	
	void setPosition(pair<double, double> position) {
		x_ = position.first;
		y_ = position.second;
		
		set = true;
	}
	
	pair<double, double> getFinalPosition() {
		if (set)
			return {x_, y_};
			
		return possible_placements.front().getFinalPosition();
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
	
	void addPossible(const Point& point) {
		possible_placements.push_back(point);
	}
	
	void addPossible(const vector<Point>& possibles) {
		possible_placements.insert(possible_placements.end(), possibles.begin(), possibles.end());
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
	vector<Point> possible_placements;
};

double distanceBetweenPoints(double x1, double y1, double x2, double y2) {
	return sqrt((x2 - x1) * (x2 - x1) + (y2 - y1) * (y2 - y1));
}

vector<Point> getSinglePossibles(Point& point, double actual_distance) {
	vector<Point> possibles;
	
	for (double i = -g_maxDistance; i < g_maxDistance; i += POINT_ACCURACY) {
		for (double j = -g_maxDistance; j < g_maxDistance; j += POINT_ACCURACY) {
			double distance = distanceBetweenPoints(i, j, point.x_, point.y_);
			double compared = abs(distance - actual_distance);
			
			if (compared < DISTANCE_ACCURACY)
				possibles.push_back(Point(i, j));
		}
	}
	
	return possibles;
}

vector<Point> getMultiplePossibles(Point& point, double actual_distance) {
	if (point.set)
		return getSinglePossibles(point, actual_distance);
		
	vector<Point> possibles;
		
	for (auto& master : point.possible_placements) {
		auto master_possibles = getSinglePossibles(master, actual_distance);
		
		possibles.insert(possibles.end(), master_possibles.begin(), master_possibles.end());
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
	
	for (auto& master : points) {
		double distance = master.getDistance(i);
		auto master_possibles = getMultiplePossibles(master, distance);
		
		possibles.insert(possibles.end(), master_possibles.begin(), master_possibles.end());
	}
	
	removeDuplicates(possibles);
	
	//cout << "After duplicates: " << possibles.size() << endl;
	
	//cout << points.front().getX() << " " << points.front().getY() << endl;
	//cout << points.front().getDistance(i) << endl;
	
	for (auto& point : possibles) {
		bool good = true;
		
		for (auto& origin : points) {
			double distance = origin.getDistance(i);
			if (abs(distance - distanceBetweenPoints(point.getX(), point.getY(), origin.getX(), origin.getY())) > DISTANCE_ACCURACY) {
				good = false;
				
				break;
			}
		}
		
		if (good) {
			working.push_back(point);
		}
	}
	
	return working;
}

vector<Point> getPlacement(vector<Point> points, size_t start) {
	vector<Point> origins(points.begin(), points.begin() + start);
	
	/*
	cout << "start: " << start << endl;
	cout << "points.size(): " << points.size() << endl;
	cout << "origins.size(): " << origins.size() << endl;
	*/
	
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

int main() {
	size_t num_points;
	cin >> num_points;
	
	vector<Point> points;
	
	for (size_t i = 0; i < num_points; i++) {
		Point point;
		
		for (size_t j = 0; j < num_points; j++) {
			double distance;
			cin >> distance;
			
			point.addDistance(distance);
			
			printf("%zu -> %zu\t= %1.2f\n", i, j, distance);
			
			if (distance > g_maxDistance)
				g_maxDistance = distance;
		}
		
		points.push_back(point);
	}
	
	g_maxDistance *= 1.5;
	cout << "Set max distance to: " << g_maxDistance << endl;
	
	points.front().setPosition({ 0, 0 });
	
	auto results = getPlacement(points, 1);
	
	for (auto& point : results) {
		if (!point.isSet())
			break;
			
		auto position = point.getFinalPosition();
		
		printf("(%1.2f, %1.2f)\n", position.first, position.second);
	}
	
	return 0;
}