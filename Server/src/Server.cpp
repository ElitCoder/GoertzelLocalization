#include "NetworkCommunication.h"
#include "Handle.h"
#include "SpeakerPlacement.h"
#include "Config.h"

#include <iostream>
#include <algorithm>

using namespace std;

enum {
	PACKET_GET_SPEAKER_VOLUME_AND_CAPTURE = 1,
	PACKET_SET_SPEAKER_VOLUME_AND_CAPTURE,
	PACKET_START_LOCALIZATION,
	PACKET_TEST_SPEAKER_DBS,
	PACKET_PARSE_SERVER_CONFIG
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
			vector<int> boosts;
			
			int num_ips = input_packet.getInt();
			
			for (int i = 0; i < num_ips; i++) {
				ips.push_back(input_packet.getString());
				volumes.push_back(input_packet.getFloat());
				captures.push_back(input_packet.getFloat());
				boosts.push_back(input_packet.getInt());
			}
			
			auto finished = Handle::handleSetSpeakerVolumeAndCapture(ips, volumes, captures, boosts);
			
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
				
			vector<SpeakerPlacement> placements = Handle::handleRunLocalization(ips, Config::get<bool>("no_scripts"));
			
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
		
		case PACKET_TEST_SPEAKER_DBS: {
			vector<string> ips;
			int num_ips = input_packet.getInt();
			
			for (int i = 0; i < num_ips; i++)
				ips.push_back(input_packet.getString());
				
			int play_time = input_packet.getInt();
			int idle_time = input_packet.getInt();
			int num_external = input_packet.getInt();
			
			for (int i = 0; i < num_external; i++)
				ips.push_back(input_packet.getString());
				
			auto results = Handle::handleTestSpeakerdBs(ips, play_time, idle_time, num_external, Config::get<bool>("no_scripts"));
			
			Packet packet;
			packet.addHeader(PACKET_TEST_SPEAKER_DBS);
			packet.addInt(results.size());
			
			for (size_t i = 0; i < results.size(); i++) {
				auto& result = results.at(i);
				auto& ip = ips.at(i);
				
				packet.addString(ip);
				packet.addInt(result.size());
				
				for (auto& db : result) {
					packet.addString(db.first);
					packet.addFloat(db.second);
				}
			}
			
			packet.finalize();
			
			network.addOutgoingPacket(connection.getSocket(), packet);
			break;	
		}
		
		case PACKET_PARSE_SERVER_CONFIG: {
			Config::clear();
			Config::parse("config");
			
			Packet packet;
			packet.addHeader(PACKET_PARSE_SERVER_CONFIG);
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
	Config::parse("config");
	
	cout << "Starting server at port " << Config::get<unsigned short>("port") << endl;
	start(Config::get<unsigned short>("port"));
	
	return 0;
}