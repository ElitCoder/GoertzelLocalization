#include "Settings.h"

#include <libnessh/SSHMaster.h>

#include <iostream>
#include <fstream>
#include <algorithm>

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
	cout << "\t-a,\t\twhich file to read addresses from (default addresses)\n";
	cout << "\t-c,\t\twhich file to read commands from (default commands)\n";
	cout << "\t-p,\t\twhile file to read users & passwords from (default passwords)\n";
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

ostream& operator<<(ostream& out, const vector<string>& elements) {
	for_each(elements.begin(), elements.end(), [] (const string& line) { cout << line << endl; });
	
	return out;
}

int execute(SSHMaster& master, vector<string>& ips, vector<string>& commands) {
	master.setSetting(SETTING_ENABLE_SSH_OUTPUT, true);
	
	for (auto command : commands) {
		while (command.back() == ';' || command.back() == ' ')
			command.pop_back();
			
		command += "; wait";
		
		cout << "Information: running command " << command << endl;
		
		vector<string> current_commands(commands.size(), command);
		
		if (!master.command(ips, current_commands)) {
			master.setSetting(SETTING_ENABLE_SSH_OUTPUT, false);
			
			return -1;
		}
	}
	
	master.setSetting(SETTING_ENABLE_SSH_OUTPUT, false);
	
	return 0;
}

void run(const string& filename_addresses, const string& filename_commands, const string& filename_passwords) {
	vector<string> ips = read(filename_addresses);
	vector<string> commands = read(filename_commands);
	vector<string> users_passwords = read(filename_passwords);
	
	vector<string> users = getUsers(users_passwords);
	vector<string> passwords = getPasswords(users_passwords);
	
	SSHMaster master;
	
	cout << ips << endl;
	cout << users << endl;
	cout << passwords << endl;
	
	if (!master.connect(ips, users, passwords))
		ERROR("could not connect");
		
	cout << "Information: connected to all devices without any problems\n";
	cout << "Information: running commands synchronized one-by-one..\n";
	
	if (execute(master, ips, commands) < 0)
		cout << "Warning: not all commands were executed due to some errors\n";
	else
		cout << "Information: all commands were executed\n";
}

int main(int argc, char** argv) {
	Settings settings;
	
	if (!settings.readParameters(argc, argv) || settings.get<bool>("-h")) {
		help();
		
		return 0;
	}
	
	string addresses = (settings.has("-a") ? settings.get<string>("-a") : "addresses");
	string commands = (settings.has("-c") ? settings.get<string>("-c") : "commands");
	string passwords = (settings.has("-p") ? settings.get<string>("-p") : "passwords");
	
	cout << "Debug: got addresses file " << addresses << endl;
	cout << "Debug: got commands file " << commands << endl;
	cout << "Debug: got passwords file " << passwords << endl;
	
	run(addresses, commands, passwords);
	
	return 0;
}