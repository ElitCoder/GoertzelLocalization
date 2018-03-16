#ifndef SETTINGS_H
#define SETTINGS_H

#include <string>
#include <vector>
#include <iostream>

class Settings {
public:
	Settings();
	
	void set(const std::string& setting, const std::string& value);
	bool has(const std::string& setting);
	std::pair<std::string, std::string>* find(const std::string& setting);
	
	template<class T>
	T get(const std::string& setting) {
		auto* where = find(setting);
		
		if (where == nullptr) {
			std::cout << "Warning: could not find option " << setting << "\n";
			
			return T();
		}
			
		std::stringstream converter(where->second);
		T value;
		converter >> value;
		
		return value;
	}
	
private:
	// TODO: change to hashmap
	std::vector<std::pair<std::string, std::string>> settings_;
};

#endif