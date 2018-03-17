#include "Settings.h"

#include <libnessh/SSHMaster.h>

#include <iostream>
#include <fstream>

#define ERROR(...)	do { fprintf(stderr, "Error: "); fprintf(stderr, __VA_ARGS__); fprintf(stderr, "\n"); exit(1); } while(0)

using namespace std;

enum {
	GET_USERS,
	GET_PASSWORDS
};

void help() {
	cout << "Usage: nessh-cli [-options]\n\n";
	cout << "Parallel SSH for running synchronized commands.\n\n";
	cout << "Options:\n";
	cout << "\t-a,\t\twhich file to read addresses from\n";
	cout << "\t-c,\t\twhich file to read commands from\n";
	cout << "\t-p,\t\twhile file to read users & passwords from\n";
}

vector<string> read(const string& filename) {
	ifstream file(filename);
	
	if (!file.is_open())
		ERROR("could not open file: %s", filename.c_str());
		
	vector<string> lines;
	string line;
	
	while (getline(file, line))
		lines.push_back(line);
		
	file.close();
		
	return lines;
}

static vector<string> getTokens(string input, char delimiter) {
	istringstream stream(input);
	vector<string> tokens;
	string token;
	
	while (getline(stream, token, delimiter))
		if (!token.empty())
			tokens.push_back(token);
	
	return tokens;
}

vector<string> getUsers(const vector<string>& users_passwords, int type = GET_USERS) {
	vector<string> information;
	
	for (auto& line : users_passwords) {
		vector<string> tokens = getTokens(line, ' ');
		
		switch (type) {
			case GET_USERS: information.push_back(tokens.front());
				break;
				
			case GET_PASSWORDS: information.push_back(tokens.back());
				break;
		}
	}
	
	return information;
}

vector<string> getPasswords(const vector<string>& users_passwords) {
	return getUsers(users_passwords, GET_PASSWORDS);
}

void run(const string& filename_addresses, const string& filename_commands, const string& filename_passwords) {
	vector<string> ips = read(filename_addresses);
	vector<string> commands = read(filename_commands);
	vector<string> users_passwords = read(filename_passwords);
	
	vector<string> users = getUsers(users_passwords);
	vector<string> passwords = getPasswords(users_passwords);
	
	SSHMaster master;
	
	if (!master.connect(ips, users, passwords))
		exit(1);
		
	cout << "Info: connected to devices\n";
}

int main(int argc, char** argv) {
	Settings settings;
	
	if (!settings.readParameters(argc, argv) || !settings.has("-a") || !settings.has("-c") || !settings.has("-p")) {
		help();
		
		return 0;
	}
	
	string addresses = settings.get<string>("-a");
	string commands = settings.get<string>("-c");
	string passwords = settings.get<string>("-p");
	
	cout << "Debug: got addresses file " << addresses << endl;
	cout << "Debug: got commands file " << commands << endl;
	cout << "Debug: got passwords file " << passwords << endl;
	
	run(addresses, commands, passwords);
	
	return 0;
}