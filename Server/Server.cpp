#include "NetworkCommunication.h"
#include "Handle.h"

#include <iostream>

using namespace std;

enum {
	PACKET_GET_SPEAKER_VOLUME_AND_CAPTURE = 1
};

static vector<string> g_ips = { "172.25.9.38", "172.25.13.200" };

void handle(NetworkCommunication& network, Connection& connection, Packet& input_packet) {
	auto header = input_packet.getByte();
	
	printf("Debug: got packet with header %02X\n", header);
	
	switch (header) {
		case 0x00: {
			string name = input_packet.getString();
			
			cout << "Information: client name is " << name << endl;
			
			Packet returning_packet;
			returning_packet.addHeader(0x00);
			returning_packet.addBool(true);
			returning_packet.finalize();
			
			network.addOutgoingPacket(connection.getSocket(), returning_packet);
			cout << "Information: returned OKAY\n";
			
			break;
		}
		
		case PACKET_GET_SPEAKER_VOLUME_AND_CAPTURE: {
			auto volumes = Handle::handleGetSpeakerVolumeAndCapture(g_ips);
			
			Packet packet;
			packet.addHeader(PACKET_GET_SPEAKER_VOLUME_AND_CAPTURE);
			packet.addInt(volumes.size());
						
			for (size_t i = 0; i < g_ips.size(); i++) {
				auto& values = volumes.at(i);
				packet.addString(g_ips.at(i));
				packet.addInt(values.size());
				
				for (auto& value : values)
					packet.addFloat(value);
			}
				
			packet.finalize();
						
			network.addOutgoingPacket(connection.getSocket(), packet);
			
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