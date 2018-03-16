#include "NetworkCommunication.h"

#include <iostream>

using namespace std;

enum {
	PACKET_GET_SPEAKER_VOLUME_AND_CAPTURE = 1
};

static NetworkCommunication* g_network;

/*
void handle(Packet* packet) {
	cout << "Debug: got packet: ";
	
	const auto* data = packet->getData();
	
	for (size_t i = 0; i < packet->getSize(); i++)
		printf("%02x ", data[i]);
		
	cout << endl;	
}
*/

void getSpeakerSettings() {
	cout << "Building request packet and sending.. ";
	
	Packet packet;
	packet.addHeader(PACKET_GET_SPEAKER_VOLUME_AND_CAPTURE);
	packet.finalize();
	
	g_network->pushOutgoingPacket(packet);
	
	cout << "done!\nWaiting for response.. ";
	Packet answer = g_network->waitForIncomingPacket();
	cout << "done!\n\n";
	answer.getByte();
	
	int num_ips = answer.getInt();
	
	for (int i = 0; i < num_ips; i++) {
		string ip = answer.getString();
		int num_values = answer.getInt();
		
		cout << "Values for " << ip << ": ";
		
		for (int j = 0; j < num_values; j++) {
			float value = answer.getFloat();
			
			cout << value << " ";
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
		cout << "\n: ";
		
		int input;
		cin >> input;
		
		cout << endl;
		
		switch (input) {
			case 1: getSpeakerSettings();
				break;
		}
		
		cout << endl;
	}
	
	/*
	while (true) {
		auto* packet = network.getIncomingPacket();
		
		if (packet == nullptr)
			continue;
			
		handle(packet);
		
		network.popIncomingPacket();	
	}
	*/
	
	g_network = nullptr;
}

int main() {
	const string HOST = "localhost";
	const unsigned short PORT = 10200;
	
	run(HOST, PORT);
	
	return 0;
}