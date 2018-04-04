#include "System.h"

// libcurlpp
#include <curlpp/cURLpp.hpp>
#include <curlpp/Options.hpp>
#include <curlpp/Easy.hpp>

#include <algorithm>
#include <sstream>

using namespace std;

Speaker& System::addSpeaker(Speaker& speaker) {
	cout << "Adding & connecting speaker " << speaker.getIP() << endl;
	
	// Enable SSH
	enableSSH({ speaker.getIP() });
	
	// Open connection to Speaker if it's offline
	speaker.setOnline(ssh_.connect(speaker.getIP(), "pass"));
	
	if (!speaker.isOnline())
		cout << "Warning: speaker " << speaker.getIP() << " is not online\n";
	
	// Add speaker to list
	speakers_.push_back(speaker);
	
	// Return reference to pushed back speaker
	return speakers_.back();
}

// Batch check for connectivity
bool System::checkConnection(const vector<string>& ips) {
	auto speakers = getSpeakers(ips);
	
	for (auto* speaker : speakers)
		if (!speaker->isOnline())
			return false;

	return true;
}

SSHOutput System::runScript(const vector<string>& ips, const vector<string>& scripts) {
	// Make sure all speakers are connected
	checkConnection(ips);
	
	for (size_t i = 0; i < ips.size(); i++) {
		cout << "SSH: running script (" << ips.at(i) << ")\n**************\n";
		cout << scripts.at(i) << "**************\n\n";
	}
	
	cout << "Running SSH commands... " << flush;
	
	ssh_.setSetting(SETTING_ENABLE_SSH_OUTPUT_VECTOR_STYLE, true);
	auto outputs = ssh_.command(ips, scripts);
	ssh_.setSetting(SETTING_ENABLE_SSH_OUTPUT_VECTOR_STYLE, false);
	
	if (outputs.empty())
		cout << "ERROR\n";
	else
		cout << "done\n";
	
	return outputs;
}

vector<Speaker*> System::getSpeakers(const vector<string>& ips) {
	vector<Speaker*> speakers;
	
	for (auto& ip : ips)
		speakers.push_back(&getSpeaker(ip));
		
	return speakers;
}

Speaker& System::getSpeaker(const string& ip) {
	auto iterator = find(speakers_.begin(), speakers_.end(), ip);
	
	if (iterator == speakers_.end()) {
		Speaker speaker;
		speaker.setIP(ip);
		
		return addSpeaker(speaker);
	} else {
		return *iterator;
	}
}

bool System::sendFile(const vector<string>& ips, const string& from, const string& to) {
	checkConnection(ips);
	
	cout << "Sending file " << from << " -> " << to << "... " << flush;
	auto status = ssh_.transferRemote(ips, vector<string>(ips.size(), from), vector<string>(ips.size(), to));
	cout << (status ? "done\n" : "ERROR\n");
	
	return status;
}

bool System::getFile(const vector<string>& ips, const vector<string>& from, const vector<string>& to) {
	checkConnection(ips);
	
	for (size_t i = 0; i < ips.size(); i++) {
		cout << "Retrieving (" << ips.at(i) << ") " << from.at(i) << " -> " << to.at(i) << endl;
	}
	
	cout << "Retrieving files from SSH... " << flush;
	auto status = ssh_.transferLocal(ips, from, to, true);
	cout << (status ? "done\n" : "ERROR\n");
	
	return status;
}

bool System::getRecordings(const vector<string>& ips) {
	vector<string> from;
	vector<string> to;
	
	for (auto& ip : ips) {
		from.push_back("/tmp/cap" + ip + ".wav");
		to.push_back("results");
	}
	
	return getFile(ips, from, to);
}

void System::enableSSH(const vector<string>& ips) {
	for (auto& ip : ips) {
		cout << "Enabling SSH in " << ip << "... " << flush;
		curlpp::Cleanup clean;
		string disable_string = "http://";
		disable_string += ip;
		disable_string += "/axis-cgi/admin/param.cgi?action=update&Network.SSH.Enabled=yes";
		
		curlpp::Easy request;
		ostringstream stream;
		
		request.setOpt(curlpp::options::Url(disable_string)); 
		request.setOpt(curlpp::options::UserPwd(string("root:pass")));
		request.setOpt(curlpp::options::HttpAuth(CURLAUTH_ANY));
		request.setOpt(curlpp::options::WriteStream(&stream));
		
		request.perform();
				
		if (stream.str() != "OK")
			cout << "ERROR\n";
		else
			cout << "done\n";
	}
}