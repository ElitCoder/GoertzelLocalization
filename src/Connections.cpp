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

SSH& Connections::getSession(const string& ip, bool threading) {
	if (threading)
		threaded_connections_mutex_.lock();
		
	auto iterator = find(connections_.begin(), connections_.end(), ip);
	
	if (iterator == connections_.end())
		ERROR("could not find session");
		
	if (threading)
		threaded_connections_mutex_.unlock();
		
	return *iterator;
}

void transferLocalThreaded(Connections& connections, const string& ip, const string& from, const string& to) {
	auto& session = connections.getSession(ip, true);
	bool result = session.transferLocal(from, to);
	
	if (result)
		return;
		
	connections.setThreadedConnectionStatus(false);
}

bool Connections::transferLocal(vector<string>& ips, vector<string>& from, vector<string>& to, bool threading) {
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
		for (size_t i = 0; i < ips.size(); i++) {
			auto& session = getSession(ips.at(i), threading);
			
			if (!session.transferLocal(from.at(i), to.at(i)))
				return false;
		}
		
		return true;
	}
}

void transferRemoteThreaded(Connections& connections, const string& ip, const string& from, const string& to) {
	auto& session = connections.getSession(ip, true);
	bool result = session.transferRemote(from, to);
	
	if (result)
		return;
		
	connections.setThreadedConnectionStatus(false);
}

bool Connections::transferRemote(vector<string>& ips, vector<string>& from, vector<string>& to) {
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

void Connections::setThreadedConnectionStatus(bool status) {
	lock_guard<mutex> guard(threaded_connections_mutex_);
	threaded_connections_result_ = status;
}

void connectThreaded(Connections& connections, const string& ip, const string& pass) {
	bool result = connections.connect(ip, pass);
	
	if (result)
		return;
		
	connections.setThreadedConnectionStatus(false);
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

void commandThreaded(Connections& connections, const string& ip, const string& command) {
	auto& session = connections.getSession(ip, true);
	bool result = session.command(command);
	
	if (result)
		return;
		
	connections.setThreadedConnectionStatus(false);	
}

bool Connections::command(vector<string>& ips, vector<string>& commands) {
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