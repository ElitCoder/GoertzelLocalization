#include "NetworkCommunication.h"

#include <iostream>

using namespace std;

enum {
	PACKET_GET_SPEAKER_VOLUME_AND_CAPTURE = 1,
	PACKET_SET_SPEAKER_VOLUME_AND_CAPTURE
};

static NetworkCommunication* g_network;
static vector<string> g_ips = { "172.25.11.186", "172.25.9.38" };

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

void getSpeakerSettings() {
	Packet packet;
	packet.addHeader(PACKET_GET_SPEAKER_VOLUME_AND_CAPTURE);
	packet.addInt(g_ips.size());
	
	for (auto& ip : g_ips)
		packet.addString(ip);
		
	packet.finalize();
	g_network->pushOutgoingPacket(packet);
	
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
	
	Packet packet;
	packet.addHeader(PACKET_SET_SPEAKER_VOLUME_AND_CAPTURE);
	packet.addInt(g_ips.size());
	
	for (size_t i = 0; i < g_ips.size(); i++) {
		packet.addString(g_ips.at(i));
		packet.addFloat(volumes.at(i));
		packet.addFloat(captures.at(i));
	}
	
	packet.finalize();
	g_network->pushOutgoingPacket(packet);
	
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

void run(const string& host, unsigned short port) {
	cout << "Connecting to server.. ";
	NetworkCommunication network(host, port);
	cout << "done!\n\n";
	
	g_network = &network;
	
	while (true) {
		cout << "1. Get speaker settings\n";
		cout << "2. Set speaker settings\n";
		cout << "\n: ";
		
		int input;
		cin >> input;
		
		cout << endl;
		
		switch (input) {
			case 1: getSpeakerSettings();
				break;
				
			case 2: setSpeakerSettings();
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