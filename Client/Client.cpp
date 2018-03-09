#include "NetworkCommunication.h"

#include <iostream>

using namespace std;

void handle(Packet* packet) {
	cout << "Debug: got packet: ";
	
	const auto* data = packet->getData();
	
	for (size_t i = 0; i < packet->getSize(); i++)
		printf("%02x ", data[i]);
		
	cout << endl;	
}

void run(const string& host, unsigned short port) {
	NetworkCommunication network(host, port);
	
	while (true) {
		auto* packet = network.getIncomingPacket();
		
		if (packet == nullptr)
			continue;
			
		handle(packet);
		
		network.popIncomingPacket();	
	}
}

int main() {
	const string HOST = "localhost";
	const unsigned short PORT = 10200;
	
	run(HOST, PORT);
	
	return 0;
}