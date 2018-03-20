#include "NetworkCommunication.h"

#include <iostream>

using namespace std;

enum {
	PACKET_GET_SPEAKER_VOLUME_AND_CAPTURE = 1,
	PACKET_SET_SPEAKER_VOLUME_AND_CAPTURE,
	PACKET_START_LOCALIZATION
};

enum {
	SPEAKER_MIN_VOLUME = 0,
	SPEAKER_MAX_VOLUME = 57,
	SPEAKER_MIN_CAPTURE = 0,
	SPEAKER_MAX_CAPTURE = 63
};

static NetworkCommunication* g_network;
static vector<string> g_ips = { "172.25.45.152",
 								"172.25.45.141",
								"172.25.45.245",
								"172.25.45.220",
								"172.25.45.109",
								"172.25.45.70",
								"172.25.45.188" };

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

Packet createSetSpeakerSettings(const vector<string>& ips, const vector<double>& volumes, const vector<double>& captures) {
	Packet packet;
	packet.addHeader(PACKET_SET_SPEAKER_VOLUME_AND_CAPTURE);
	packet.addInt(ips.size());
	
	for (size_t i = 0; i < ips.size(); i++) {
		packet.addString(ips.at(i));
		packet.addFloat(volumes.at(i));
		packet.addFloat(captures.at(i));
	}
	
	packet.finalize();
	return packet;
}

void setSpeakerSettings() {
	vector<double> volumes;
	vector<double> captures;
	
	for (auto& ip : g_ips) {
		double tmp;
		
		cout << ip << " - volume: ";
		cin >> tmp;
		
		volumes.push_back(tmp);
		
		cout << ip << " - capture volume: ";
		cin >> tmp;
		
		captures.push_back(tmp);
	}
	
	g_network->pushOutgoingPacket(createSetSpeakerSettings(g_ips, volumes, captures));
	
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
	cout << "Setting all speakers to same volume and capture volume.. " << flush;
	g_network->pushOutgoingPacket(createSetSpeakerSettings(g_ips, vector<double>(g_ips.size(), SPEAKER_MAX_VOLUME), vector<double>(g_ips.size(), SPEAKER_MAX_CAPTURE)));
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

void run(const string& host, unsigned short port) {
	cout << "Connecting to server.. ";
	NetworkCommunication network(host, port);
	cout << "done!\n\n";
	
	g_network = &network;
	
	while (true) {
		cout << "1. Get speaker settings\n";
		cout << "2. Set speaker settings\n";
		cout << "3. Start speaker localization script\n";
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