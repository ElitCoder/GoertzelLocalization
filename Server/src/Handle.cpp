#include "Handle.h"
#include "WavReader.h"
#include "Config.h"
#include "Localization3D.h"
#include "Goertzel.h"
#include "Base.h"

#include <iostream>
#include <cmath>
#include <algorithm>
#include <climits>

using namespace std;

static SSHOutput runBasicScriptExternal(const vector<string>& speakers, const vector<string>& mics, const vector<string>& all_ips, const vector<string>& scripts, const string& send_from, const string& send_to) {
	if (!Base::system().sendFile(speakers, send_from, send_to))
		return SSHOutput();
		
	auto output = Base::system().runScript(all_ips, scripts);
	
	if (output.empty())
		return output;
		
	if (!Base::system().getRecordings(mics))
		return SSHOutput();
		
	return output;
}

static SSHOutput runBasicScript(const vector<string>& ips, const vector<string>& scripts, const string& send_from, const string& send_to) {
	return runBasicScriptExternal(ips, ips, ips, scripts, send_from, send_to);
}

static short getRMS(const vector<short>& data, size_t start, size_t end) {
	unsigned long long sum = 0;
	
	for (size_t i = start; i < end; i++)
		sum += (data.at(i) * data.at(i));
		
	sum /= (end - start);
	
	return sqrt(sum);
}

bool Handle::setSpeakerAudioSettings(const vector<string>& ips, const vector<int>& volumes, const vector<int>& captures, const vector<int>& boosts) {
	vector<string> commands;
	
	for (size_t i = 0; i < volumes.size(); i++) {
		string volume = to_string(volumes.at(i));
		string capture = to_string(captures.at(i));
		string boost = to_string(boosts.at(i));
		
		string command = "amixer -c1 sset 'Headphone' " + volume + "; wait; ";
		command += "amixer -c1 sset 'Capture' " + capture + "; wait; ";
		command += "dspd -s -m; wait; dspd -s -u limiter; wait; ";
		command += "dspd -s -u preset; wait; dspd -s -p flat; wait; ";
		command += "amixer -c1 sset 'PGA Boost' " + boost + "; wait\n";
		
		commands.push_back(command);
	}
	
	auto status = !Base::system().runScript(ips, commands).empty();
	
	if (status) {
		for (size_t i = 0; i < ips.size(); i++) {
			auto& speaker = Base::system().getSpeaker(ips.at(i));
			
			speaker.setVolume(volumes.at(i));
			speaker.setMicVolume(captures.at(i));
			speaker.setMicBoost(boosts.at(i));
			speaker.setEQ(vector<int>(9, 0));
		}	
	}
	
	return status;
}

static vector<string> createRunLocalizationScripts(const vector<string>& ips, int play_time, int idle_time, int extra_recording, const string& file) {
	vector<string> scripts;
	
	for (size_t i = 0; i < ips.size(); i++) {
		string script =	"systemctl stop audio*\n";
		script +=		"arecord -D audiosource -r 48000 -f S16_LE -c 1 -d ";
		script +=		to_string(idle_time * 2 + ips.size() * (1 + play_time) + extra_recording - 1 /* no need for one extra second */);
		script +=		" /tmp/cap";
		script +=		ips.at(i);
		script +=		".wav &\n";
		script +=		"proc1=$!\n";
		script +=		"sleep ";
		script +=		to_string(idle_time + i * (play_time + 1));
		script +=		"\n";
		script +=		"aplay -D localhw_0 -r 48000 -f S16_LE /tmp/";
		script += 		file;
		script +=		"\n";
		script +=		"wait $proc1\n";
		script +=		"systemctl start audio-conf; wait\n";
		
		scripts.push_back(script);
	}
	
	return scripts;
}

void printSSHOutput(SSHOutput outputs) {
	for (auto& output : outputs)
		for (auto& line : output.second)
			cout << "SSH (" << output.first << "): " << line << endl;
}

static PlacementOutput assemblePlacementOutput(const vector<Speaker*> speakers) {
	PlacementOutput output;
	
	for (auto* speaker : speakers) {
		auto ip = speaker->getIP();
		const auto& coordinates = speaker->getPlacement().getCoordinates();
		const auto& distances = speaker->getPlacement().getDistances();
		
		output.push_back(make_tuple(ip, coordinates, distances));
	}
	
	return output;
}

PlacementOutput Handle::runLocalization(const vector<string>& ips, bool skip_script, bool force_update) {
	if (ips.empty())
		return PlacementOutput();

	// Does the server already have relevant positions?
	vector<int> placement_ids;
	
	for (auto& ip : ips)
		placement_ids.push_back(Base::system().getSpeaker(ip).getPlacementID());
	
	if (adjacent_find(placement_ids.begin(), placement_ids.end(), not_equal_to<int>()) == placement_ids.end() && placement_ids.front() >= 0 && !force_update) {
		cout << "Server already have relevant position info, returning that\n";
		
		return assemblePlacementOutput(Base::system().getSpeakers(ips));
	}
	
	if (!skip_script) {
		// Create scripts
		int play_time = Config::get<int>("speaker_play_length");
		int idle_time = Config::get<int>("idle_time");
		int extra_recording = Config::get<int>("extra_recording");
		
		auto scripts = createRunLocalizationScripts(ips, play_time, idle_time, extra_recording, Config::get<string>("goertzel"));
		
		if (runBasicScript(ips, scripts, "data/" + Config::get<string>("goertzel"), "/tmp/").empty())
			return PlacementOutput();
	}
	
	auto distances = Goertzel::runGoertzel(ips);
	
	if (distances.empty())
		return PlacementOutput();
	
	auto placement = Localization3D::run(distances, Config::get<bool>("fast"));
	
	// Keep track of which localization this is
	static int placement_id = -1;
	placement_id++;
	
	cout << "Setting placement ID to " << placement_id << endl;
	
	for (size_t i = 0; i < ips.size(); i++) {
		Speaker::SpeakerPlacement speaker_placement(ips.at(i));
		auto& master = distances.at(i);
		
		for (size_t j = 0; j < master.second.size(); j++)
			speaker_placement.addDistance(ips.at(j), master.second.at(j));

		speaker_placement.setCoordinates(placement.at(i));
		
		Base::system().getSpeaker(ips.at(i)).setPlacement(speaker_placement, placement_id);
	}
	
	return assemblePlacementOutput(Base::system().getSpeakers(ips));
}

vector<bool> Handle::checkSpeakersOnline(const vector<string>& ips) {
	vector<bool> online;
	
	for_each(ips.begin(), ips.end(), [&online] (auto& ip) {
		online.push_back(Base::system().checkConnection({ ip }));
	});
	
	return online;
}

static vector<string> createSoundImageScripts(const vector<string>& speakers, const vector<string>& mics, int play_time, int idle_time, const string& filename) {
	vector<string> scripts;
	
	for (size_t i = 0; i < speakers.size(); i++) {
		string script = "systemctl stop audio*\n";
		script +=		"sleep " + to_string(idle_time) + "\n";
		script +=		"aplay -D localhw_0 -r 48000 -f S16_LE /tmp/" + filename + "\n";
		script +=		"systemctl start audio-conf; wait\n";
		
		scripts.push_back(script);
	}
	
	for (size_t i = 0; i < mics.size(); i++) {
		string script = "systemctl stop audio*\n";
		script +=		"arecord -D audiosource -r 48000 -f S16_LE -c 1 -d " + to_string(idle_time * 2 + play_time);
		script +=		" /tmp/cap";
		script +=		mics.at(i);
		script +=		".wav\n";
		script +=		"systemctl start audio-conf; wait\n";
		
		scripts.push_back(script);
	}
	
	return scripts;
}

// Taken from git/SO
static double goertzel(int numSamples,float TARGET_FREQUENCY,int SAMPLING_RATE, short* data)
{
    int     k,i;
    float   floatnumSamples;
    float   omega,sine,cosine,coeff,q0,q1,q2,magnitude,real,imag;

    float   scalingFactor = numSamples / 2.0;

    floatnumSamples = (float) numSamples;
    k = (int) (0.5 + ((floatnumSamples * TARGET_FREQUENCY) / (float)SAMPLING_RATE));
    omega = (2.0 * M_PI * k) / floatnumSamples;
    sine = sin(omega);
    cosine = cos(omega);
    coeff = 2.0 * cosine;
    q0=0;
    q1=0;
    q2=0;

    for(i=0; i<numSamples; i++)
    {
        q0 = coeff * q1 - q2 + data[i];
        q2 = q1;
        q1 = q0;
    }

    // calculate the real and imaginary results
    // scaling appropriately
    real = (q1 - q2 * cosine) / scalingFactor;
    imag = (q2 * sine) / scalingFactor;

    magnitude = sqrtf(real*real + imag*imag);
    return magnitude;
}

static vector<double> getFFT9(const vector<short>& data, size_t start, size_t end) {
	vector<short> sound(data.begin() + start, data.begin() + end);
	
	// Do FFT here - scratch that, let's do 9 Goertzel calculations instead
	double freq63 = goertzel(sound.size(), 63, 48000, sound.data());
	double freq125 = goertzel(sound.size(), 125, 48000, sound.data());
	double freq250 = goertzel(sound.size(), 250, 48000, sound.data());
	double freq500 = goertzel(sound.size(), 500, 48000, sound.data());
	double freq1000 = goertzel(sound.size(), 1000, 48000, sound.data());
	double freq2000 = goertzel(sound.size(), 2000, 48000, sound.data());
	double freq4000 = goertzel(sound.size(), 4000, 48000, sound.data());
	double freq8000 = goertzel(sound.size(), 8000, 48000, sound.data());
	double freq16000 = goertzel(sound.size(), 16000, 48000, sound.data());
	
	return { freq63, freq125, freq250, freq500, freq1000, freq2000, freq4000, freq8000, freq16000 };
} 

template<class T>
static T getMean(const vector<T>& container) {
	T sum = 0;
	
	for(const auto& element : container)
		sum += element;
		
	return sum / container.size();	
}

static double getSoundImageScore(const vector<double>& dbs, double mean_db) {
	//double mean = mean_db < 0 ? getMean(dbs) : mean_db;
	double mean = mean_db < 0 ? getMean(dbs) : getMean(dbs);
	double score = 0;
	
	for (const auto& db : dbs)
		score += (mean - db) * (mean - db);
	
	return 1 / sqrt(score);
}

static vector<int> getSoundImageCorrection(vector<double> dbs) {
	// Abs everything
	for_each(dbs.begin(), dbs.end(), [] (auto& db) { db = abs(db); });
	
	double min = *min_element(dbs.begin(), dbs.end());
	vector<int> correction_eq;
	
	for (auto& db : dbs) {
		double correction = db - min;
		
		correction_eq.push_back(lround(correction));
	}
	
	/*
	// Let's just go for flat right now
	double mean = getMean(dbs);
	vector<int> correction_eq;
	
	for (auto& db : dbs) {
		double correction = db - mean;
		
		correction_eq.push_back(lround(correction));
	}
	*/
	
	// Fixed in Speaker
	/*
	// Make sure we don't overload the DSP	
	for (auto& setting : correction_eq) {
		if (setting < -10)
			setting = -10;
		else if (setting > 10)
			setting = 10;
	}
	*/
		
	return correction_eq;
}

static void setCorrectedEQ(const vector<string>& ips) {
	auto speakers = Base::system().getSpeakers(ips);
	vector<string> commands;
	
	for (auto* speaker : speakers) {
		auto& correction_eq = speaker->getCorrectionEQ();
		//vector<int> correction_eq = { 9, -10, 0, 0, 0, 0, 0, 0, 0 };
		
		string command =	"dspd -s -u preset; wait; ";
		command +=			"dspd -s -e ";
		
		for (auto setting : correction_eq)
			command += to_string(setting) + ",";
			
		command.pop_back();	
		command +=			"; wait\n";
		
		commands.push_back(command);
	}
	
	Base::system().runScript(ips, commands);
}

static void setFlatEQ(const vector<string>& ips) {
	auto speakers = Base::system().getSpeakers(ips);
	vector<string> commands;
	
	for (auto* speaker : speakers) {
		vector<int> correction_eq(9, 0);
		speaker->setEQ(correction_eq);
		
		// Also set correction EQ to flat since we're restarting
		speaker->setCorrectionEQ(correction_eq);
		
		string command =	"dspd -s -u preset; wait; ";
		command +=			"dspd -s -e ";
		
		for (auto setting : correction_eq)
			command += to_string(setting) + ",";
			
		command.pop_back();	
		command +=			"; wait\n";
		
		commands.push_back(command);
	}
	
	Base::system().runScript(ips, commands);
}

SoundImageFFT9 Handle::checkSoundImage(const vector<string>& speakers, const vector<string>& mics, int play_time, int idle_time, bool corrected) {
	// Set corrected EQ if we're trying the corrected sound image or restore flat settings
	if (corrected) {
		setCorrectedEQ(speakers);
	} else {
		setFlatEQ(speakers);
	}
	
	if (!Config::get<bool>("no_scripts")) {
		// Get sound image from available microphones
		auto scripts = createSoundImageScripts(speakers, mics, play_time, idle_time, Config::get<string>("white_noise"));
		
		vector<string> all_ips(speakers);
		all_ips.insert(all_ips.end(), mics.begin(), mics.end());
		
		if (runBasicScriptExternal(speakers, mics, all_ips, scripts, "data/" + Config::get<string>("white_noise"), "/tmp/").empty())
			return SoundImageFFT9();
	}
		
	SoundImageFFT9 final_result;
		
	for (size_t i = 0; i < mics.size(); i++) {
		string filename = "results/cap" + mics.at(i) + ".wav";
		
		vector<short> data;
		WavReader::read(filename, data);
		
		if (data.empty())
			return SoundImageFFT9();
		
		size_t sound_start = ((double)idle_time + 0.3) * 48000;
		size_t sound_average = getRMS(data, sound_start, sound_start + (48000 / 2));
		
		//cout << "Debug: sound_average " << sound_average << endl;
		
		double db = 20 * log10(sound_average / (double)SHRT_MAX);
		
		//cout << "Debug: db " << db << endl;
		
		// Calculate FFT for 9 band as well
		auto db_fft = getFFT9(data, sound_start, sound_start + (48000 / 2));
		vector<double> dbs;
		
		for (auto& freq : db_fft) {
			double db_freq = 20 * log10(freq / (double)SHRT_MAX);
			
			dbs.push_back(db_freq);
		}
		
		// Calculate score
		auto score = getSoundImageScore(dbs, Base::system().getSpeaker(speakers.front()).getTargetMeanDB());
		
		// Calculate correction & set it to speakers (alpha)
		auto correction = getSoundImageCorrection(dbs);
		
		for (auto* speaker : Base::system().getSpeakers(speakers)) {
			if (!corrected) {
				// Set flat results if not set
				speaker->setFlatResults(dbs);
			} else {
				auto flat = speaker->getFlatResults();
				auto& set = dbs;
				
				// Now - before
				for (size_t d = 0; d < dbs.size(); d++) {
					cout << flat.at(d) << " -> " << set.at(d) << "\tdifference: " << (set.at(d) - flat.at(d)) << endl;
					cout << "Corrected was: " << speaker->getCorrectionEQ().at(d) << endl;
				}
			}
			
			auto actual_new_eq = speaker->setCorrectionEQ(correction);
			//speaker->setTargetMeanDB(correction.first);
		}
		
		cout << "Current score: " << score << endl;
		
		// 9 band dB first, then time domain dB
		dbs.push_back(db);
		
		final_result.push_back(make_tuple(mics.at(i), dbs, score));
	}	
		
	return final_result;
}

#if 0
bool Handle::setEQ(const vector<string>& speakers, const vector<double>& settings) {
	cout << "NOT IMPLEMENTED\n";
	
	return false;
	
	/*
	string command =	"dspd -s -u preset; wait; ";
	command +=			"dspd -s -e ";
	
	for (auto setting : settings)
		command += to_string(lround(setting)) + ",";
		
	command.pop_back();	
	command +=			"; wait";
	
	return !Base::system().runScript(speakers, vector<string>(speakers.size(), command)).empty();
	*/
}
#endif