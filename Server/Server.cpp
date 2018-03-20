#include "NetworkCommunication.h"
#include "Handle.h"
#include "SpeakerPlacement.h"

#include <iostream>
#include <algorithm>

using namespace std;

enum {
	PACKET_GET_SPEAKER_VOLUME_AND_CAPTURE = 1,
	PACKET_SET_SPEAKER_VOLUME_AND_CAPTURE,
	PACKET_START_LOCALIZATION
};

static void handle(NetworkCommunication& network, Connection& connection, Packet& input_packet) {
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
			vector<string> ips;
			int num_ips = input_packet.getInt();
			
			for (int i = 0; i < num_ips; i++)
				ips.push_back(input_packet.getString());
				
			auto finished = Handle::handleGetSpeakerVolumeAndCapture(ips);
			
			Packet packet;
			packet.addHeader(PACKET_SET_SPEAKER_VOLUME_AND_CAPTURE);
			packet.addInt(finished.size());
			
			for (size_t i = 0; i < finished.size(); i++) {
				auto& ip = finished.at(i).first;
				auto& lines = finished.at(i).second;
				
				packet.addString(ip);
				packet.addInt(lines.size());
				
				for (auto& line : lines)
					packet.addString(line);
			}
			
			packet.finalize();
			
			network.addOutgoingPacket(connection.getSocket(), packet);
			break;
		}
		
		case PACKET_SET_SPEAKER_VOLUME_AND_CAPTURE: {
			vector<string> ips;
			vector<double> volumes;
			vector<double> captures;
			
			int num_ips = input_packet.getInt();
			
			for (int i = 0; i < num_ips; i++) {
				ips.push_back(input_packet.getString());
				volumes.push_back(input_packet.getFloat());
				captures.push_back(input_packet.getFloat());
			}
			
			auto finished = Handle::handleSetSpeakerVolumeAndCapture(ips, volumes, captures);
			
			Packet packet;
			packet.addHeader(PACKET_SET_SPEAKER_VOLUME_AND_CAPTURE);
			packet.addInt(finished.size());
			
			for (size_t i = 0; i < finished.size(); i++) {
				auto& ip = finished.at(i).first;
				auto& lines = finished.at(i).second;
				
				packet.addString(ip);
				packet.addInt(lines.size());
				
				for (auto& line : lines)
					packet.addString(line);
			}
			
			packet.finalize();
			
			network.addOutgoingPacket(connection.getSocket(), packet);
			break;
		}
		
		case PACKET_START_LOCALIZATION: {
			vector<string> ips;
			int num_ips = input_packet.getInt();
			
			for (int i = 0; i < num_ips; i++)
				ips.push_back(input_packet.getString());
				
			vector<SpeakerPlacement> placements = Handle::handleRunLocalization(ips);
			
			Packet packet;
			packet.addHeader(PACKET_START_LOCALIZATION);
			packet.addInt(placements.size());
			
			for (auto& speaker : placements) {
				packet.addString(speaker.getIp());
				
				auto& coordinates = speaker.getCoordinates();
				packet.addInt(coordinates.size());
				for_each(coordinates.begin(), coordinates.end(), [&packet] (double c) { packet.addFloat(c); });
				
				auto& distances = speaker.getDistances();
				packet.addInt(distances.size());
				for_each(distances.begin(), distances.end(), [&packet] (const pair<string, double>& peer) {
					packet.addString(peer.first);
					packet.addFloat(peer.second);
				});
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

static void start(unsigned short port) {
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