#include "Connections.h"

#include <libssh/callbacks.h>

#include <algorithm>
#include <iostream>
#include <thread>

#define ERROR(...)	do { fprintf(stderr, "Error: "); fprintf(stderr, __VA_ARGS__); fprintf(stderr, "\n"); exit(1); } while(0)

using namespace std;

Connections::Connections() {
	ssh_threads_set_callbacks(ssh_threads_get_pthread());
	ssh_init();
}

Connections::~Connections() {
	for_each(connections_.begin(), connections_.end(), [] (SSH& session) { session.disconnect(); });
}

bool Connections::connect(const string& ip, const string& pass) {
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

void Connections::setThreadedConnectionStatus(bool status) {
	lock_guard<mutex> guard(threaded_connections_mutex_);
	threaded_connections_result_ = status;
}

bool connectThreaded(Connections& connections, const string& ip, const string& pass) {
	bool result = connections.connect(ip, pass);
	
	if (result)
		return true;
		
	connections.setThreadedConnectionStatus(false);
	
	return false;
}

bool Connections::connect(const vector<string>& ips, const string& pass) {
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

bool Connections::command(const string& ip, const string& command) {
	auto iterator = find(connections_.begin(), connections_.end(), ip);
	
	if (iterator == connections_.end()) {
		cout << ip << " is not connected\n";
		
		return false;
	}
	
	SSH& session = *iterator;
	
	return session.command(command);
}