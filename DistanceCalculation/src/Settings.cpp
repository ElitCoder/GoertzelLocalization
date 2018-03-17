#include "Settings.h"

#include <algorithm>
#include <iostream>

using namespace std;

Settings::Settings() {
}

pair<string, string>* Settings::find(const string& setting) {
	auto where = find_if(settings_.begin(), settings_.end(), [&setting] (pair<string, string>& option) { return option.first == setting; });
	
	if (where == settings_.end())
		return nullptr;
		
	return &(*where);
}

void Settings::set(const string& setting, const string& value) {
	//cout << "Debug: setting option " << setting << " to " << value << endl;
	
	auto* option = find(setting);
	
	if (option == nullptr)
		settings_.push_back({ setting, value });
	else
	 	option->second = value;
}

bool Settings::has(const string& setting) {
	auto* option = find(setting);
	
	return option != nullptr;
}