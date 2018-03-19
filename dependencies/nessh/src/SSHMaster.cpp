#include "SSHMaster.h"

#include <libssh/callbacks.h>

#include <algorithm>
#include <iostream>
#include <thread>

#define ERROR(...)	do { fprintf(stderr, "Error: "); fprintf(stderr, __VA_ARGS__); fprintf(stderr, "\n"); exit(1); } while(0)

using namespace std;

SSHMaster::SSHMaster() :
	settings_(SETTING_MAX, false) {
	ssh_threads_set_callbacks(ssh_threads_get_pthread());
	ssh_init();
}

SSHMaster::~SSHMaster() {
	for_each(connections_.begin(), connections_.end(), [] (SSH& session) { session.disconnect(); });
}

void SSHMaster::setSetting(int setting, bool value) {
	settings_.at(setting) = value;
}

bool SSHMaster::getSetting(int setting) {
	return settings_.at(setting);
}

bool SSHMaster::connect(const string& ip, const string& pass) {
	{
		lock_guard<mutex> guard(threaded_connections_mutex_);
		
		if (find(connections_.begin(), connections_.end(), ip) != connections_.end()) {
			cout << ip << " is already connected!\n";
			
			return false;
		}
	}
	
	SSH session(ip, pass);
	
	if (session.connect()) {
		lock_guard<mutex> guard(threaded_connections_mutex_);
		connections_.push_back(session);
		
		return true;
	} else {
		return false;
	}
}

bool SSHMaster::connect(const string& ip, const string& user, const string& pass) {
	{
		lock_guard<mutex> guard(threaded_connections_mutex_);
		
		if (find(connections_.begin(), connections_.end(), ip) != connections_.end()) {
			cout << ip << " is already connected!\n";
			
			return false;
		}
	}
	
	SSH session(ip, user, pass);
	
	if (session.connect()) {
		lock_guard<mutex> guard(threaded_connections_mutex_);
		connections_.push_back(session);
		
		return true;
	} else {
		return false;
	}
}

SSH& SSHMaster::getSession(const string& ip, bool threading) {
	if (threading)
		threaded_connections_mutex_.lock();
		
	auto iterator = find(connections_.begin(), connections_.end(), ip);
	
	if (iterator == connections_.end())
		ERROR("could not find session");
		
	if (threading)
		threaded_connections_mutex_.unlock();
		
	return *iterator;
}

static void transferLocalThreaded(SSHMaster& connections, const string& ip, const string& from, const string& to) {
	auto& session = connections.getSession(ip, true);
	bool result = session.transferLocal(from, to, connections.getSetting(SETTING_USE_ACTUAL_FILENAME) ? to : "");
	
	if (result)
		return;
		
	connections.setThreadedConnectionStatus(false);
}

bool SSHMaster::transferLocal(vector<string>& ips, vector<string>& from, vector<string>& to, bool threading) {
	if (ips.empty())
		return false;
		
	if (threading) {
		threaded_connections_result_ = true;
		
		thread* threads = new thread[ips.size()];
		
		for (size_t i = 0; i < ips.size(); i++)
			threads[i] = thread(transferLocalThreaded, ref(*this), ref(ips.at(i)), ref(from.at(i)), ref(to.at(i)));
			
		for (size_t i = 0; i < ips.size(); i++)
			threads[i].join();
			
		delete[] threads;
		
		return threaded_connections_result_;
	} else {
		// TODO: remove this condition
		
		ERROR("no threading specified in transferLocal, someone forgot to remote this");
		
		/*
		for (size_t i = 0; i < ips.size(); i++) {
			auto& session = getSession(ips.at(i), threading);
			
			if (!session.transferLocal(from.at(i), to.at(i)))
				return false;
		}
		
		return true;
		*/
	}
}

static void transferRemoteThreaded(SSHMaster& connections, const string& ip, const string& from, const string& to) {
	auto& session = connections.getSession(ip, true);
	bool result = session.transferRemote(from, to);
	
	if (result)
		return;
		
	connections.setThreadedConnectionStatus(false);
}

bool SSHMaster::transferRemote(vector<string>& ips, vector<string>& from, vector<string>& to) {
	if (ips.empty())
		return false;
		
	threaded_connections_result_ = true;
	
	thread* threads = new thread[ips.size()];
	
	for (size_t i = 0; i < ips.size(); i++)
		threads[i] = thread(transferRemoteThreaded, ref(*this), ref(ips.at(i)), ref(from.at(i)), ref(to.at(i)));
		
	for (size_t i = 0; i < ips.size(); i++)
		threads[i].join();
		
	delete[] threads;
	
	return threaded_connections_result_;	
}

void SSHMaster::setThreadedConnectionStatus(bool status) {
	lock_guard<mutex> guard(threaded_connections_mutex_);
	threaded_connections_result_ = status;
}

static void connectThreaded(SSHMaster& connections, const string& ip, const string& pass) {
	bool result = connections.connect(ip, pass);
	
	if (result)
		return;
		
	connections.setThreadedConnectionStatus(false);
}

static void connectThreadedUser(SSHMaster& connections, const string& ip, const string& user, const string& pass) {
	bool result = connections.connect(ip, user, pass);
	
	if (result)
		return;
		
	connections.setThreadedConnectionStatus(false);
}

bool SSHMaster::connect(const vector<string>& ips, const string& pass) {
	if (ips.empty())
		return false;
		
	threaded_connections_result_ = true;
		
	thread* threads = new thread[ips.size()];
		
	for (size_t i = 0; i < ips.size(); i++) {
		threads[i] = thread(connectThreaded, ref(*this), ref(ips.at(i)), ref(pass));
	}
	
	for (size_t i = 0; i < ips.size(); i++) {
		threads[i].join();
	}
	
	delete[] threads;
	
	return threaded_connections_result_;
}

bool SSHMaster::connect(const vector<string>& ips, const vector<string>& users, const vector<string>& passwords) {
	if (ips.empty() || users.empty() || passwords.empty())
		return false;
		
	threaded_connections_result_ = true;
		
	thread* threads = new thread[ips.size()];
		
	for (size_t i = 0; i < ips.size(); i++) {
		threads[i] = thread(connectThreadedUser, ref(*this), ref(ips.at(i)), ref(users.at(i)), ref(passwords.at(i)));
	}
	
	for (size_t i = 0; i < ips.size(); i++) {
		threads[i].join();
	}
	
	delete[] threads;
	
	return threaded_connections_result_;	
}

static void commandThreaded(SSHMaster& connections, const string& ip, const string& command) {
	auto& session = connections.getSession(ip, true);
	bool result = session.command(command);
	
	if (result)
		return;
		
	connections.setThreadedConnectionStatus(false);	
}

bool SSHMaster::command(vector<string>& ips, vector<string>& commands) {
	if (ips.empty())
		return false;
		
	threaded_connections_result_ = true;
		
	thread* threads = new thread[ips.size()];
			
	for (size_t i = 0; i < ips.size(); i++)
		threads[i] = thread(commandThreaded, ref(*this), ref(ips.at(i)), ref(commands.at(i)));
		
	for (size_t i = 0; i < ips.size(); i++)
		threads[i].join();
		
	delete[] threads;
	
	return threaded_connections_result_;	
			
			/*
	auto iterator = find(connections_.begin(), connections_.end(), ip);
	
	if (iterator == connections_.end()) {
		cout << ip << " is not connected\n";
		
		return false;
	}
	
	SSH& session = *iterator;
	
	return session.command(command);
	*/
}