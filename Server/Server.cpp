#include "NetworkCommunication.h"

#include <iostream>

using namespace std;

void handle(Connection& connection, Packet& packet) {
	if (connection.getSocket()) { }
		
	switch (packet.getByte()) {
		case 0x01: {
			cout << "Debug: got update packet!\n";
			
			break;
		}
		
		default: {
			cout << "Debug: got some random packet\n";
		}
	}
}

void start(unsigned short port) {
	NetworkCommunication network(port);
	
	Packet packet;
	packet.addHeader(0x00);
	packet.finalize();
	
	while (true) {
		network.addOutgoingPacketToAllExcept(packet, {});
				
		this_thread::sleep_for(chrono::seconds(1));
		
		/*
		auto* packet = network.waitForProcessingPackets();
		
		if (packet == nullptr)
			continue;
			
		auto* connection_pair = network.getConnectionAndLock(packet->first);
		
		if (connection_pair == nullptr) {
			network.removeProcessingPacket();
			
			continue;
		}
			
		cout << "Got packet!\n";
		
		handle(connection_pair->second, packet->second);
		
		network.unlockConnection(*connection_pair);
		network.removeProcessingPacket();
		*/
	}
}

int main() {
	const unsigned short PORT = 10200;
	
	cout << "Starting server at port " << PORT << endl;
	start(PORT);
	
	return 0;
}