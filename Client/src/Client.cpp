#include "NetworkCommunication.h"

#include <iostream>

using namespace std;

enum {
	PACKET_GET_SPEAKER_VOLUME_AND_CAPTURE = 1,
	PACKET_SET_SPEAKER_VOLUME_AND_CAPTURE,
	PACKET_START_LOCALIZATION,
	PACKET_TEST_SPEAKER_DBS,
	PACKET_PARSE_SERVER_CONFIG
};

enum {
	SPEAKER_MIN_VOLUME = 0,				// 0 = -57 dB
	SPEAKER_MAX_VOLUME = 57,			// 57 = 0 dB
	SPEAKER_MAX_VOLUME_SAFE = 51,		// 51 = -6 dB
	SPEAKER_MIN_CAPTURE = 0,			// 0 = -12 dB
	SPEAKER_MAX_CAPTURE = 63,			// 63 = 35.25 dB
	SPEAKER_CAPTURE_BOOST_MUTE = 0,		// -inf dB
	SPEAKER_CAPTURE_BOOST_NORMAL = 1,	// 0 dB
	SPEAKER_CAPTURE_BOOST_ENABLED = 2	// +20 dB
};

static NetworkCommunication* g_network;

// Våning 3
static vector<string> g_ips = { "172.25.9.38",
								"172.25.13.200" };
							 	//"172.25.11.98" };
							 	//"172.25.14.27" };
// J0
/*
static vector<string> g_ips = { "172.25.45.152",
 								"172.25.45.141",
								"172.25.45.245",
								"172.25.45.220",
								"172.25.45.109",
								"172.25.45.70",
								"172.25.45.188" };
								*/
								
// External microphones
static vector<string> g_external_microphones = {};								

using SSHOutput = vector<pair<string, vector<string>>>;

SSHOutput getSSHOutputFromPacket(Packet& packet) {
	SSHOutput outputs;
	int num_ips = packet.getInt();
	
	for (int i = 0; i < num_ips; i++) {
		string ip = packet.getString();
		int num_lines = packet.getInt();
		
		vector<string> lines;
		
		for (int j = 0; j < num_lines; j++)
			lines.push_back(packet.getString());
			
		outputs.push_back({ ip, lines });	
	}
	
	return outputs;
}

Packet createGetSpeakerSettings(const vector<string>& ips) {
	Packet packet;
	packet.addHeader(PACKET_GET_SPEAKER_VOLUME_AND_CAPTURE);
	packet.addInt(ips.size());
	
	for (auto& ip : ips)
		packet.addString(ip);
		
	packet.finalize();
	return packet;
}

void getSpeakerSettings() {
	g_network->pushOutgoingPacket(createGetSpeakerSettings(g_ips));
	
	Packet answer = g_network->waitForIncomingPacket();
	answer.getByte();
	
	auto outputs = getSSHOutputFromPacket(answer);
	
	for (auto& output : outputs) {
		auto& ip = output.first;
		auto& lines = output.second;
		
		cout << "Output for " << ip << ":\n";
		
		for (auto& line : lines)
			cout << "\t" << line << endl;
	}
}

// TODO: remove horn IP
Packet createSetSpeakerSettings(const vector<string>& ips, const vector<double>& volumes, const vector<double>& captures, const vector<double>& boosts) {
	Packet packet;
	packet.addHeader(PACKET_SET_SPEAKER_VOLUME_AND_CAPTURE);
	packet.addInt(ips.size());
	
	for (size_t i = 0; i < ips.size(); i++) {
		//if (ips.at(i) == "172.25.11.98")
		//	continue;
			
		packet.addString(ips.at(i));
		packet.addFloat(volumes.at(i));
		packet.addFloat(captures.at(i));
		packet.addInt(boosts.at(i));
	}
	
	packet.finalize();
	return packet;
}

void setSpeakerSettings() {
	vector<double> volumes;
	vector<double> captures;
	vector<double> boosts;
	
	for (auto& ip : g_ips) {
		double tmp;
		int tmp_int;
		
		cout << ip << " - volume (" << SPEAKER_MIN_VOLUME << " - " << SPEAKER_MAX_VOLUME << ", SAFE MAX = " << SPEAKER_MAX_VOLUME_SAFE << "): ";
		cin >> tmp;
		volumes.push_back(tmp);
		
		cout << ip << " - capture volume (" << SPEAKER_MIN_CAPTURE << " - " << SPEAKER_MAX_CAPTURE << "): ";
		cin >> tmp;
		captures.push_back(tmp);
		
		cout << ip << " - boost (" << SPEAKER_CAPTURE_BOOST_MUTE << " - " << SPEAKER_CAPTURE_BOOST_ENABLED << "): ";
		cin >> tmp_int;
		boosts.push_back(tmp_int);
	}
	
	g_network->pushOutgoingPacket(createSetSpeakerSettings(g_ips, volumes, captures, boosts));
	
	cout << endl;
	
	Packet answer = g_network->waitForIncomingPacket();
	// We already know the header
	answer.getByte();
	
	int num_ips = answer.getInt();
	
	for (int i = 0; i < num_ips; i++) {
		string ip = answer.getString();
		int num_lines = answer.getInt();
		
		cout << "Output for " << ip << ":\n";
		
		for (int j = 0; j < num_lines; j++) {
			cout << "\t" << answer.getString() << endl;
		}
	}
}

Packet createStartSpeakerLocalization(const vector<string>& ips) {
	Packet packet;
	packet.addHeader(PACKET_START_LOCALIZATION);
	packet.addInt(ips.size());
	
	for (auto& ip : ips)
		packet.addString(ip);
		
	packet.finalize();
	return packet;
}

void startSpeakerLocalization() {
	// Set speakers to same volume
	cout << "\nSetting all speakers to same volume and capture volume.. " << flush;
	g_network->pushOutgoingPacket(createSetSpeakerSettings(g_ips, vector<double>(g_ips.size(), SPEAKER_MAX_VOLUME), vector<double>(g_ips.size(), SPEAKER_MAX_CAPTURE), vector<double>(g_ips.size(), SPEAKER_CAPTURE_BOOST_ENABLED)));
	g_network->waitForIncomingPacket();
	cout << "done\n";
	
	cout << "Running speaker localization script.. " << flush;
	g_network->pushOutgoingPacket(createStartSpeakerLocalization(g_ips));
	
	auto answer = g_network->waitForIncomingPacket();
	answer.getByte();
	cout << "done\n\n";
	
	// Parse data
	int num_speakers = answer.getInt();
	
	for (int i = 0; i < num_speakers; i++) {
		string own_ip = answer.getString();
		cout << own_ip << ": ";
		
		int num_dimensions = answer.getInt();
		
		cout << "(";
		for (int j = 0; j < num_dimensions; j++) {
			cout << answer.getFloat();
			
			if (j + 1 != num_dimensions)
				cout << ", ";
		}
		cout << ")\n";
			
		cout << "With distances:\n";
		int num_distances = answer.getInt();
		
		for (int j = 0; j < num_distances; j++) {
			string ip = answer.getString();
			double distance = answer.getFloat();
			
			cout << own_ip << " -> " << ip << "\t= " << distance << " m\n";
		}
		
		cout << endl;	
	}
}

void setMaxSpeakerSettings() {
	cout << "Running script.. " << flush;
	g_network->pushOutgoingPacket(createSetSpeakerSettings(g_ips, vector<double>(g_ips.size(), SPEAKER_MAX_VOLUME), vector<double>(g_ips.size(), SPEAKER_MAX_CAPTURE), vector<double>(g_ips.size(), SPEAKER_CAPTURE_BOOST_ENABLED)));
	g_network->waitForIncomingPacket();
	cout << "done!\n\n";
}

void printIPs() {
	for (auto& ip : g_ips) {
		cout << ip << endl;
	}
	
	cout << endl;
}

Packet createSpeakerdB(vector<string>& ips, vector<string>& external_ips) {
	Packet packet;
	packet.addHeader(PACKET_TEST_SPEAKER_DBS);
	packet.addInt(ips.size());
	
	for (auto& ip : ips)
		packet.addString(ip);
		
	packet.addInt(1);	// Play time
	packet.addInt(2);	// Idle time
	packet.addInt(external_ips.size());
	
	for (auto& ip : external_ips)
		packet.addString(ip);
		
	packet.finalize();
	return packet;
}

void speakerdB() {
	cout << "Setting all speakers to -10 dB.. " << flush;
	g_network->pushOutgoingPacket(createSetSpeakerSettings(g_ips, vector<double>(g_ips.size(), SPEAKER_MAX_VOLUME - 10), vector<double>(g_ips.size(), SPEAKER_MAX_CAPTURE), vector<double>(g_ips.size(), SPEAKER_CAPTURE_BOOST_ENABLED)));
	g_network->waitForIncomingPacket();
	cout << "done!\n";
	
	cout << "Running remote scripts and collecting data.. " << flush;
	g_network->pushOutgoingPacket(createSpeakerdB(g_ips, g_external_microphones));
	Packet answer = g_network->waitForIncomingPacket();
	cout << "done!\n\n";
	answer.getByte();
	
	int num_speakers = answer.getInt();
	
	for (int i = 0; i < num_speakers; i++) {
		string ip = answer.getString();
		int num_db = answer.getInt();
		
		for (int j = 0; j < num_db; j++) {
			string playing_ip = answer.getString();
			double db = answer.getFloat();
			
			cout << ip << " <- " << playing_ip << "\t: " << db << " dB\n";
		}
	}
	
	cout << endl;
}

void firstInstall() {
	cout << "Not yet implemented." << endl;
	
	// TODO: implement this
}

Packet createParseServerConfig() {
	Packet packet;
	packet.addHeader(PACKET_PARSE_SERVER_CONFIG);
	packet.finalize();
	
	return packet;
}

void parseServerConfig() {
	cout << "Rebuild config files.. ";
	g_network->pushOutgoingPacket(createParseServerConfig());
	g_network->waitForIncomingPacket();
	cout << "done!\n\n";
}

void run(const string& host, unsigned short port) {
	cout << "Connecting to server.. ";
	NetworkCommunication network(host, port);
	cout << "done!\n\n";
	
	g_network = &network;
	
	while (true) {
		cout << "1. Get speaker settings\n";
		cout << "2. Set speaker settings\n";
		cout << "3. Start speaker localization script\n";
		cout << "4. Set max safe volume & capture + boost for all speakers\n";
		cout << "5. See current IPs\n";
		cout << "6. See how speakers affect eachother dB\n";
		cout << "7. First install, enable SSH & factory reset & flash latest alpha\n";
		cout << "8. Parse Server config again\n";
		cout << "\n: ";
		
		int input;
		cin >> input;
		
		cout << endl;
		
		switch (input) {
			case 1: getSpeakerSettings();
				break;
				
			case 2: setSpeakerSettings();
				break;
				
			case 3: startSpeakerLocalization();
				break;
				
			case 4: setMaxSpeakerSettings();
				break;
				
			case 5: printIPs();
				break;
				
			case 6: speakerdB();
				break;
				
			case 7: firstInstall();
				break;
				
			case 8: parseServerConfig();
				break;
				
			default: cout << "Invalid input\n";
		}
		
		cout << endl;
	}
	
	g_network = nullptr;
}

int main() {
	const string HOST = "localhost";
	const unsigned short PORT = 10200;
	
	run(HOST, PORT);
	
	return 0;
}