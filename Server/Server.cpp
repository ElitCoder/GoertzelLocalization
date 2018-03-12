#include "NetworkCommunication.h"

#include <iostream>

using namespace std;

void handle(NetworkCommunication& network, Connection& connection, Packet& packet) {
	switch (packet.getByte()) {
		case 0x00: {
			string name = packet.getString();
			
			cout << "Information: client name is " << name << endl;
			
			Packet returning_packet;
			returning_packet.addHeader(0x00);
			returning_packet.addBool(true);
			returning_packet.finalize();
			
			network.addOutgoingPacket(connection.getSocket(), returning_packet);
			cout << "Information: returned OKAY\n";
			
			break;
		}
		
		default: {
			cout << "Debug: got some random packet\n";
		}
	}
}

void start(unsigned short port) {
	NetworkCommunication network(port);
	
	while (true) {
		auto* packet = network.waitForProcessingPackets();
		
		if (packet == nullptr)
			continue;
			
		auto* connection_pair = network.getConnectionAndLock(packet->first);
		
		if (connection_pair == nullptr) {
			network.removeProcessingPacket();
			
			continue;
		}
			
		cout << "Got packet!\n";
		
		handle(network, connection_pair->second, packet->second);
		
		network.unlockConnection(*connection_pair);
		network.removeProcessingPacket();
	}
}

int main() {
	const unsigned short PORT = 10200;
	
	cout << "Starting server at port " << PORT << endl;
	start(PORT);
	
	return 0;
}