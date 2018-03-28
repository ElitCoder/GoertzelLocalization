#include "NetworkCommunication.h"

#include <iostream>
#include <algorithm>
#include <cmath>

using namespace std;

enum {
	PACKET_GET_SPEAKER_VOLUME_AND_CAPTURE = 1,
	PACKET_SET_SPEAKER_VOLUME_AND_CAPTURE,
	PACKET_START_LOCALIZATION,
	PACKET_TEST_SPEAKER_DBS,
	PACKET_PARSE_SERVER_CONFIG,
	PACKET_CHECK_SPEAKERS_ONLINE,
	PACKET_CHECK_SOUND_IMAGE,
	PACKET_CHECK_OWN_SOUND_LEVEL
};

enum {
	SPEAKER_MIN_VOLUME = 0,				// 0 = -57 dB
	SPEAKER_MAX_VOLUME = 57,			// 57 = 0 dB
	SPEAKER_MIN_CAPTURE = 0,			// 0 = -12 dB
	SPEAKER_MAX_CAPTURE = 63,			// 63 = 35.25 dB
	SPEAKER_CAPTURE_BOOST_MUTE = 0,		// -inf dB
	SPEAKER_CAPTURE_BOOST_NORMAL = 1,	// 0 dB
	SPEAKER_CAPTURE_BOOST_ENABLED = 2	// +20 dB
};

static NetworkCommunication* g_network;

// Speakers
static vector<string> g_ips = { "172.25.14.27", "172.25.13.200" };
// External microphones
static vector<string> g_external_microphones = { "172.25.9.38", "172.25.11.186" };

Packet createSetSpeakerSettings(const vector<string>& ips, const vector<int>& volumes, const vector<int>& captures, const vector<int>& boosts) {
	Packet packet;
	packet.addHeader(PACKET_SET_SPEAKER_VOLUME_AND_CAPTURE);
	packet.addInt(ips.size());
	
	for (size_t i = 0; i < ips.size(); i++) {
		packet.addString(ips.at(i));
		packet.addInt(volumes.at(i));
		packet.addInt(captures.at(i));
		packet.addInt(boosts.at(i));
	}
	
	packet.finalize();
	return packet;
}

// Should this include microphones?
Packet createStartSpeakerLocalization(const vector<string>& ips) {
	Packet packet;
	packet.addHeader(PACKET_START_LOCALIZATION);
	packet.addInt(ips.size());
	
	for (auto& ip : ips)
		packet.addString(ip);
		
	packet.finalize();
	return packet;
}

static void setSpeakerSettings(int speaker_volume, int speaker_capture, int speaker_boost) {
	vector<string> all_ips(g_ips);
	all_ips.insert(all_ips.end(), g_external_microphones.begin(), g_external_microphones.end());
	
	g_network->pushOutgoingPacket(createSetSpeakerSettings(all_ips, vector<int>(all_ips.size(), speaker_volume), vector<int>(all_ips.size(), speaker_capture), vector<int>(all_ips.size(), speaker_boost)));
	g_network->waitForIncomingPacket();
}

static void setMaxSpeakerSettings() {
	setSpeakerSettings(SPEAKER_MAX_VOLUME, SPEAKER_MAX_CAPTURE, SPEAKER_CAPTURE_BOOST_ENABLED);
}

void startSpeakerLocalization() {
	cout << "Setting max speaker settings.. " << flush;
	setMaxSpeakerSettings();
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

void startSpeakerLocalizationAll() {
	cout << "Setting max speaker settings.. " << flush;
	setMaxSpeakerSettings();
	cout << "done\n";
	
	cout << "Running speaker localization script.. " << flush;
	vector<string> all_ips(g_ips);
	all_ips.insert(all_ips.end(), g_external_microphones.begin(), g_external_microphones.end());
	
	g_network->pushOutgoingPacket(createStartSpeakerLocalization(all_ips));
	
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

Packet createSpeakerdB(vector<string>& ips, vector<string>& external_ips, bool normalize) {
	Packet packet;
	packet.addHeader(PACKET_TEST_SPEAKER_DBS);
	packet.addInt(ips.size());
	
	for (auto& ip : ips)
		packet.addString(ip);
		
	packet.addInt(1);	// Play time
	packet.addInt(2);	// Idle time
	packet.addInt(external_ips.size());
	packet.addBool(normalize);
	
	for (auto& ip : external_ips)
		packet.addString(ip);
		
	packet.finalize();
	return packet;
}

void speakerdB() {
	cout << "Setting normal speaker settings.. " << flush;
	setSpeakerSettings(SPEAKER_MAX_VOLUME, SPEAKER_MAX_CAPTURE, SPEAKER_CAPTURE_BOOST_NORMAL);
	cout << "done\n";
	
	cout << "Running decibel test and collecting data.. " << flush;
	g_network->pushOutgoingPacket(createSpeakerdB(g_ips, g_external_microphones, true));
	Packet answer = g_network->waitForIncomingPacket();
	cout << "done!\n\n";
	answer.getByte();
	
	int num_speakers = answer.getInt();
	
	for (int i = 0; i < num_speakers; i++) {
		string ip = answer.getString();
		int num_db = answer.getInt();
		
		double all_db = 0;
		
		for (int j = 0; j < num_db; j++) {
			string playing_ip = answer.getString();
			double db = answer.getFloat();
			
			cout << ip << " <- " << playing_ip << "\t: " << db << " dB\n";
			
			all_db += db;
		}
		
		all_db /= num_db;
		
		cout << "Total " << ip << "\t\t: " << all_db << " dB\n";
	}
	
	cout << endl;
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

Packet createCheckSpeakerOnline(const vector<string>& ips) {
	Packet packet;
	packet.addHeader(PACKET_CHECK_SPEAKERS_ONLINE);
	packet.addInt(ips.size());
	
	for (auto& ip : ips)
		packet.addString(ip);
		
	packet.finalize();
	return packet;
}

void speakersOnline() {
	vector<string> all_ips(g_ips);
	all_ips.insert(all_ips.end(), g_external_microphones.begin(), g_external_microphones.end());
	
	cout << "Trying speakers.. " << flush;
	g_network->pushOutgoingPacket(createCheckSpeakerOnline(all_ips));
	auto answer = g_network->waitForIncomingPacket();
	cout << "done!\n\n";
	
	answer.getByte();
	int num_speakers = answer.getInt();
	
	for (int i = 0; i < num_speakers; i++) {
		string ip = answer.getString();
		bool online = answer.getBool();
		
		if (find(g_external_microphones.begin(), g_external_microphones.end(), ip) != g_external_microphones.end())
			cout << "(Listening) ";
		else
			cout << "(Playing) ";
		
		cout << ip << " is " << (online ? "online" : "NOT online") << endl;
	}
	
	cout << endl;
}

Packet createSoundImage(const vector<string>& speakers, const vector<string>& mics) {
	Packet packet;
	packet.addHeader(PACKET_CHECK_SOUND_IMAGE);
	packet.addInt(speakers.size());
	packet.addInt(mics.size());
	packet.addInt(1);	// Play time
	packet.addInt(2);	// Idle time
	
	for (auto& ip : speakers)
		packet.addString(ip);
		
	for (auto& ip : mics)
		packet.addString(ip);
		
	packet.finalize();
	return packet;
}

void soundImage() {
	cout << "Setting normal speaker settings.. " << flush;
	setSpeakerSettings(SPEAKER_MAX_VOLUME, SPEAKER_MAX_CAPTURE, SPEAKER_CAPTURE_BOOST_NORMAL);
	cout << "done\n";
	
	cout << "Trying sound image.. " << flush;
	g_network->pushOutgoingPacket(createSoundImage(g_ips, g_external_microphones));
	auto answer = g_network->waitForIncomingPacket();
	answer.getByte();
	cout << "done\n\n";
	
	int num_mics = answer.getInt();
	
	for (int i = 0; i < num_mics; i++) {
		string ip = answer.getString();
		double db = answer.getFloat();
		
		cout << "Microphone " << ip << " gets " << db << " dB\n";
	}
	
	cout << endl;
}

using MicrophoneDBs = vector<pair<string, vector<pair<string, double>>>>;

MicrophoneDBs getMicrophoneDBs(Packet& packet) {
	packet.getByte();
	
	int num_speakers = packet.getInt();
	
	MicrophoneDBs mic_dbs;
	
	for (int i = 0; i < num_speakers; i++) {
		string ip = packet.getString();
		int num_db = packet.getInt();
		
		vector<pair<string, double>> dbs;
		
		for (int j = 0; j < num_db; j++) {
			string playing_ip = packet.getString();
			double db = packet.getFloat();
			
			dbs.push_back({ playing_ip, db });
		}
		
		mic_dbs.push_back({ ip, dbs });
	}
	
	return mic_dbs;
}

void checkAttenuation() {
	cout << "Setting normal speaker settings.. " << flush;
	setSpeakerSettings(SPEAKER_MAX_VOLUME, SPEAKER_MAX_CAPTURE, SPEAKER_CAPTURE_BOOST_NORMAL);
	cout << "done\n";
	
	cout << "Calculating non-normalized dB for -0 dB speaker settings.. " << flush;
	g_network->pushOutgoingPacket(createSpeakerdB(g_ips, g_external_microphones, false));
	Packet answer = g_network->waitForIncomingPacket();
	auto high = getMicrophoneDBs(answer);
	cout << "done\n\n";
	
	cout << "Setting lower speaker settings.. " << flush;
	setSpeakerSettings(SPEAKER_MAX_VOLUME - 6, SPEAKER_MAX_CAPTURE, SPEAKER_CAPTURE_BOOST_NORMAL);
	cout << "done\n";
	
	cout << "Calculating non-normalized dB for -6 dB speaker settings.. " << flush;
	g_network->pushOutgoingPacket(createSpeakerdB(g_ips, g_external_microphones, false));
	auto answer_low = g_network->waitForIncomingPacket();
	auto low = getMicrophoneDBs(answer_low);
	cout << "done\n\n";
	
	cout << "Setting lowest speaker settings.. " << flush;
	setSpeakerSettings(SPEAKER_MAX_VOLUME - 12, SPEAKER_MAX_CAPTURE, SPEAKER_CAPTURE_BOOST_NORMAL);
	cout << "done\n";
	
	cout << "Calculating non-normalized dB for -12 dB speaker settings.. " << flush;
	g_network->pushOutgoingPacket(createSpeakerdB(g_ips, g_external_microphones, false));
	auto answer_lower = g_network->waitForIncomingPacket();
	auto lower = getMicrophoneDBs(answer_lower);
	cout << "done\n\n";
	
	for (size_t i = 0; i < high.size(); i++) {
		cout << "Microphone " << high.at(i).first << endl;
		
		for (size_t j = 0; j < high.at(i).second.size(); j++) {
			double db_high = high.at(i).second.at(j).second;
			double db_low = low.at(i).second.at(j).second;
			double db_lower = lower.at(i).second.at(j).second;
			
			double change_lower_low = db_lower - db_low;
			double change_low_high = db_low - db_high;
			
			cout << "Speaker " << high.at(i).second.at(j).first << " ";
			cout << change_lower_low << " - " << change_low_high << endl;
			
			cout << "High: " << db_high << endl;
			cout << "Low: " << db_low << endl;
			cout << "Lower: " << db_lower << endl;
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
		cout << "1. Check if speakers are online\n";
		cout << "2. Reparse server config\n";
		cout << "3. Start speaker localization script (only speakers)\n";
		cout << "4. Start speaker localization script (all IPs)\n";
		cout << "5. Check speaker dB effect (currently normalized on noise)\n";
		cout << "6. Check sound image (currently normalized on noise)\n";
		cout << "7. Check attenuation for each speaker to microphone\n";
		cout << "\n: ";
		
		int input;
		cin >> input;
		
		cout << endl;
		
		switch (input) {
			case 1: speakersOnline();
				break;
				
			case 2: parseServerConfig();
				break;
				
			case 3: startSpeakerLocalization();
				break;
				
			case 4: startSpeakerLocalizationAll();
				break;
				
			case 5: speakerdB();
				break;
				
			case 6: soundImage();
				break;
				
			case 7: checkAttenuation();
				break;
				
			default: cout << "Wrong input format!\n";
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