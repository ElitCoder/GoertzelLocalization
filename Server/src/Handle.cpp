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

static vector<string> g_frequencies = {	"63",
									"125",
									"250",
									"500",
									"1000",
									"2000",
									"4000",
									"8000",
									"16000" };

static double g_target_mean = -55;

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

/* converts dB to linear gain */
double dB_to_linear_gain(double x) {
    return pow(10,x/20);
}

void to_523(double param_dec, unsigned char * param_hex) {
	long param223;
	long param227;

	//multiply decimal number by 2^23
	param223 = param_dec * (1<<23);
	// convert to positive binary
	param227 = param223 + (1<<27);

	param_hex[3] = param227 & 0xFF;       //byte 3 (LSB) of parameter value
	param_hex[2] = (param227>>8) & 0xFF;  //byte 2 of parameter value
	param_hex[1] = (param227>>16) & 0xFF; //byte 1 of parameter value
	param_hex[0] = (param227>>24) & 0xFF; //byte 0 (MSB) of parameter value

	// invert sign bit to get correct sign
	param_hex[0] = param_hex[0] ^ 0x08;
}

bool Handle::setSpeakerAudioSettings(const vector<string>& ips, const vector<int>& volumes, const vector<int>& captures, const vector<int>& boosts) {
	vector<string> commands;
	
	// -12 dB = 002026f3
	//vector<unsigned char> dsp_gain(100);
	//to_523(dB_to_linear_gain(-12), dsp_gain.data());
	
	for (size_t i = 0; i < volumes.size(); i++) {
		string volume = to_string(volumes.at(i));
		string capture = to_string(captures.at(i));
		string boost = to_string(boosts.at(i));
		
		string command = 	"dspd -s -w; wait; ";
		command +=			"amixer -c1 sset 'Headphone' " + volume + " on; wait; ";
		command +=			"amixer -c1 sset 'Capture' " + capture + "; wait; ";
		command +=			"dspd -s -m; wait; dspd -s -u limiter; wait; ";
		command +=			"dspd -s -u static; wait; ";
		command +=			"dspd -s -u preset; wait; dspd -s -p flat; wait; ";
		command +=			"amixer -c1 sset 'PGA Boost' " + boost + "; wait\n";
		
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
		script +=		"systemctl start audio_relayd; wait\n";
		
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
		string script =	"sleep " + to_string(idle_time) + "\n";
		script +=		"aplay -D localhw_0 -r 48000 -f S16_LE /tmp/" + filename + "\n";
		
		scripts.push_back(script);
	}
	
	for (size_t i = 0; i < mics.size(); i++) {
		string script =	"arecord -D audiosource -r 48000 -f S16_LE -c 1 -d " + to_string(idle_time * 2 + play_time);
		script +=		" /tmp/cap";
		script +=		mics.at(i);
		script +=		".wav\n";
		
		scripts.push_back(script);
	}
	
	return scripts;
}

static vector<string> createSoundImageIndividualScripts(const vector<string>& speakers, const vector<string>& mics, int play_time, int idle_time, const string& filename) {
	vector<string> scripts;
	
	for (size_t i = 0; i < speakers.size(); i++) {
		string script =	"sleep " + to_string(idle_time + i * (play_time + 1)) + "\n";
		script +=		"aplay -D localhw_0 -r 48000 -f S16_LE /tmp/" + filename + "\n";
		
		scripts.push_back(script);
	}
	
	for (size_t i = 0; i < mics.size(); i++) {
		string script =	"arecord -D audiosource -r 48000 -f S16_LE -c 1 -d " + to_string(idle_time + speakers.size() * (play_time + 1));
		script +=		" /tmp/cap";
		script +=		mics.at(i);
		script +=		".wav\n";
		
		scripts.push_back(script);
	}
	
	return scripts;
}

static vector<string> createDisableAudioSystem(const vector<string>& ips) {
	string command = "systemctl stop audio*; wait\n";
	
	return vector<string>(ips.size(), command);
}

static vector<string> createEnableAudioSystem(const vector<string>& ips) {
	string command = "systemctl start audio_relayd; wait\n";
	
	return vector<string>(ips.size(), command);
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

static double getSoundImageScore(const vector<double>& dbs) {
	double mean = getMean(dbs);
	double score = 0;
	
	vector<double> dbs_above_63(dbs.begin(), dbs.end());
	
	for (const auto& db : dbs_above_63)
		score += (mean - db) * (mean - db);
	
	return 1 / sqrt(score);
}

static vector<int> getSoundImageCorrection(vector<double> dbs, size_t num_speakers) {
	vector<int> eq;
	
	double mean = getMean(dbs);
	
	for (auto& db : dbs) {
		double difference = g_target_mean - db;
		
		// Adjust for DSP effect
		eq.push_back(lround(difference));
	}
	
	return eq;
	
	#if 0
	// Abs everything
	//for_each(dbs.begin(), dbs.end(), [] (auto& db) { db = abs(db); });
	vector<int> eq;
	
	for (auto& db : dbs) {
		double difference = -(db - g_target_mean);
	//	double change = 1.0 - 1 / difference;
	//	difference *= change * 1.5;
		//difference *= 1.2;
		eq.push_back(lround(difference));
	}
	
	return eq;
	#endif
	#if 0
	
	for_each(dbs.begin(), dbs.end(), [] (auto& db) { db = abs(db); });
	
	double min = *min_element(dbs.begin(), dbs.end());
	vector<int> correction_eq;
	
	for (auto& db : dbs) {
		double correction = db - min;
		
		correction_eq.push_back(lround(correction));
	}
		
	return correction_eq;
	#endif
}

static void setSpeakerVolume(const string& ip, int volume) {
	auto& speaker = Base::system().getSpeaker(ip);
	speaker.setVolume(volume);
	//speaker.setVolume(speaker.getCurrentVolume() + delta_volume);
	
	cout << "Setting speaker volume to " << speaker.getCurrentVolume() << endl;
	
	string command = "amixer -c1 sset 'Headphone' " + to_string(speaker.getCurrentVolume()) + " on; wait; ";
	Base::system().runScript({ ip }, { command });
}

static vector<double> setSpeakersBestEQ(const vector<string>& ips) {
	auto speakers = Base::system().getSpeakers(ips);
	vector<string> commands;
	vector<double> scores;
	
	for (auto* speaker : speakers) {
		auto correction_eq = speaker->getBestEQ();
		speaker->setBestVolume();
		//vector<int> correction_eq = { 9, -10, 0, 0, 0, 0, 0, 0, 0 };
		
		string command =	"dspd -s -u preset; wait; ";
		command +=			"amixer -c1 cset numid=170 0x00,0x80,0x00,0x00; wait; ";
		command +=			"dspd -s -e ";
		
		for (auto setting : correction_eq)
			command += to_string(setting) + ",";
			
		command.pop_back();	
		command +=			"; wait\n";
		
		commands.push_back(command);
		
		cout << "Best score: " << speaker->getBestScore() << endl;
		scores.push_back(speaker->getBestScore());
		
		setSpeakerVolume(speaker->getIP(), speaker->getCurrentVolume());
	}
	
	Base::system().runScript(ips, commands);
	
	return scores;
}

static void setCorrectedEQ(const vector<string>& ips) {
	auto speakers = Base::system().getSpeakers(ips);
	vector<string> commands;
	
	for (auto* speaker : speakers) {
		auto& correction_eq = speaker->getCorrectionEQ();
		speaker->setCorrectionVolume();
		//vector<int> correction_eq = { 9, -10, 0, 0, 0, 0, 0, 0, 0 };
		
		string command =	"dspd -s -u preset; wait; ";
		command +=			"dspd -s -e ";
		
		for (auto setting : correction_eq)
			command += to_string(setting) + ",";
			
		command.pop_back();	
		command +=			"; wait\n";
		
		commands.push_back(command);
		
		setSpeakerVolume(speaker->getIP(), speaker->getCurrentVolume());
	}
	
	Base::system().runScript(ips, commands);
}

static void setFlatEQ(const vector<string>& ips) {
	auto speakers = Base::system().getSpeakers(ips);
	vector<string> commands;
	
	for (auto* speaker : speakers) {
		speaker->setVolume(57);
		speaker->clearAllEQs();
		
		string command =	"dspd -s -u preset; wait; ";
		command += 			"amixer -c1 cset numid=170 0x00,0x20,0x26,0xf3; wait; ";
		command +=			"dspd -s -e ";
		
		for (auto setting : vector<int>(9, 0))
			command += to_string(setting) + ",";
			
		command.pop_back();	
		command +=			"; wait\n";
		
		commands.push_back(command);
		
		setSpeakerVolume(speaker->getIP(), speaker->getCurrentVolume());
	}
	
	Base::system().runScript(ips, commands);
}

SoundImageFFT9 Handle::checkSoundImage(const vector<string>& speakers, const vector<string>& mics, int play_time, int idle_time, bool corrected) {
	vector<string> all_ips(speakers);
	all_ips.insert(all_ips.end(), mics.begin(), mics.end());
	
	// Set corrected EQ if we're trying the corrected sound image or restore flat settings
	if (corrected) {
		setCorrectedEQ(speakers);
	} else {
		auto scripts = createDisableAudioSystem(all_ips);
		Base::system().runScript(all_ips, scripts);
		
		setFlatEQ(speakers);
		
		// Set target mean accordingly to amount of speakers
		g_target_mean = -55 + ((double)speakers.size() * 3.0);
		
		cout << "Target mean is " << g_target_mean << endl;
	}
	
	if (!Config::get<bool>("no_scripts")) {
		// Get sound image from available microphones
		vector<string> scripts;
		
		if (corrected)
			scripts = createSoundImageScripts(speakers, mics, play_time, idle_time, Config::get<string>("white_noise"));
		else
			scripts = createSoundImageIndividualScripts(speakers, mics, play_time, idle_time, Config::get<string>("white_noise"));
		
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
		
		if (corrected) {
			size_t sound_start = ((double)idle_time + 0.5) * 48000;
			size_t sound_average = getRMS(data, sound_start, sound_start + (48000 / 3));
			
			//cout << "Debug: sound_average " << sound_average << endl;
			
			double db = 20 * log10(sound_average / (double)SHRT_MAX);
			
			//cout << "Debug: db " << db << endl;
			
			// Calculate FFT for 9 band as well
			auto db_fft = getFFT9(data, sound_start, sound_start + (48000 / 2));
			vector<double> dbs;
			
			cout << "Microphone \t" << mics.at(i) << endl;
			
			//for (auto& freq : db_fft) {
			for (size_t z = 0; z < db_fft.size(); z++) {
				auto& freq = db_fft.at(z);
				double db_freq = 20 * log10(freq / (double)SHRT_MAX);
				
				cout << "Frequency " << g_frequencies.at(z) << ": \t" << db_freq << endl;
				dbs.push_back(db_freq);
			}
			
			// Calculate score
			auto score = getSoundImageScore(dbs);
			
			// Calculate correction & set it to speakers (alpha)
			auto correction = getSoundImageCorrection(dbs, speakers.size());
			
			// Correct EQ
			vector<vector<double>> incoming_dbs;
			vector<double> incoming_gains;
			//vector<double> incoming_gains;
			vector<vector<int>> corrected_dbs(speakers.size());
			
			// See all relative DBs
			for (size_t k = 0; k < speakers.size(); k++) {
				incoming_dbs.push_back(Base::system().getSpeaker(mics.at(i)).getFrequencyResponseFrom(speakers.at(k)));
				incoming_gains.push_back(Base::system().getSpeaker(mics.at(i)).getLinearGainFrom(speakers.at(k)));
			}
			
			// All energy	
			double total_gain = 0;
			
			for (auto& gain : incoming_gains)
				total_gain += gain;
				
				/*
			for (size_t s = 0; s < speakers.size(); s++){
				auto gain = Base::system().getSpeaker(mics.at(i)).getLinearGainFrom(speakers.at(s));
				double percent = gain / total_gain;
				vector<int> eq;
				for (int e = 0; e < 9; e++)
					eq.push_back(lround(percent*correction.at(e)));
				corrected_dbs.push_back(eq);
			}
			*/
			
		
			// Go through all frequency bands
			for (int d = 0; d < 9; d++) {
				double total_linear = 0;
				
				for (auto& incoming_db : incoming_dbs)
					total_linear += SHRT_MAX * dB_to_linear_gain(incoming_db.at(d));
					
				//cout << "Total linear gain: " << total_linear << endl;
					
				for (size_t e = 0; e < corrected_dbs.size(); e++) {
					double percent = (SHRT_MAX * dB_to_linear_gain(incoming_dbs.at(e).at(d))) / total_linear;
					
					// How much did we hear this speaker?
					//auto gain = Base::system().getSpeaker(mics.at(i)).getLinearGainFrom(speakers.at(e));
					//double percent_gain = gain / total_gain + 1;
					
					//percent = 1 - percent;
					//double percent = 1.0 / speakers.size();
					
					//cout << "Percent: " << percent << endl;
					
					corrected_dbs.at(e).push_back(lround((double)correction.at(d) * percent));
					
					//cout << "Which means " << ((double)correction.at(d) * percent) << " of sound\n";
				}
			}
			
			// Set EQs
			auto actual_speakers = Base::system().getSpeakers(speakers);
			
			for (size_t d = 0; d < actual_speakers.size(); d++)
				actual_speakers.at(d)->getIP(), actual_speakers.at(d)->setCorrectionEQ(corrected_dbs.at(d), score);
			
			#if 0
			for (auto* speaker : Base::system().getSpeakers(speakers)) {
				if (!corrected) {
					// Set flat results if not set
					speaker->setFlatResults(dbs);
				} else {
					auto flat = speaker->getFlatResults();
					auto& set = dbs;
					
					/*
					// Now - before
					for (size_t d = 0; d < dbs.size(); d++) {
						cout << flat.at(d) << " -> " << set.at(d) << "\tdifference: " << (set.at(d) - flat.at(d)) << endl;
						cout << "Corrected was: " << speaker->getCorrectionEQ().at(d) << endl;
					}*/
				}
				
				auto actual_new_eq = speaker->setCorrectionEQ(correction, score);
				//speaker->setTargetMeanDB(correction.first);
			}
			#endif
			
			cout << "Current score: " << score << endl;
			
			// 9 band dB first, then time domain dB
			dbs.push_back(db);
			
			final_result.push_back(make_tuple(mics.at(i), dbs, score));
		} else {
			for (size_t j = 0; j < speakers.size(); j++) {
				size_t sound_start = ((double)idle_time + j * (play_time + 1) + 0.3) * 48000;
				size_t sound_average = getRMS(data, sound_start, sound_start + (48000 / 2));
				
				//cout << "Debug: sound_average " << sound_average << endl;
				
				double db = 20 * log10(sound_average / (double)SHRT_MAX);
				
				cout << "Debug: db from " << speakers.at(j) << " is " << db << endl;
				
				// Calculate FFT for 9 band as well
				auto db_fft = getFFT9(data, sound_start, sound_start + (48000 / 2));
				vector<double> dbs;
				
				//for (auto& freq : db_fft) {
				for (size_t z = 0; z < db_fft.size(); z++) {
					auto& freq = db_fft.at(z);
					double db_freq = 20 * log10(freq / (double)SHRT_MAX);
					
					dbs.push_back(db_freq);
				}
				
				// DBs is vector of the current speaker with all it's frequency dBs
				Base::system().getSpeaker(mics.at(i)).setFrequencyResponseFrom(speakers.at(j), dbs);
				
				// Set linear gain from speaker to mic
				Base::system().getSpeaker(mics.at(i)).setLinearGainFrom(speakers.at(j), dB_to_linear_gain(db) * SHRT_MAX); 
			}
		}
	}	
		
	return final_result;
}

vector<double> Handle::setBestEQ(const vector<string>& speakers) {
	// Enable audio system again
	auto scripts = createEnableAudioSystem(speakers);
	Base::system().runScript(speakers, scripts);
	
	return setSpeakersBestEQ(speakers);
}

void Handle::setEQStatus(const vector<string>& ips, bool status) {
	string command =	"dspd -s -" + string((status ? "u" : "b"));
	command +=			" preset; wait\n";
		
	cout << "Change EQ command: " << command << endl;
	
	Base::system().runScript(ips, vector<string>(ips.size(), command));
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