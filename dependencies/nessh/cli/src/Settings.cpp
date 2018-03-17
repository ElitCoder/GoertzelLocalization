#include "Settings.h"

#include <algorithm>

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

bool Settings::readParameters(int argc, char** argv) {
	argc--;
	argv++;
	
	set("-h", "0");
	
	for (int i = 0; i < argc; i += 2) {
		if (i + 1 >= argc)
			return false;
			
		string option = argv[i];
		string value = argv[i + 1];
		
		set(option, value);
	}
	
	return true;
}