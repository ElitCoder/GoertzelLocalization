#include "SSH.h"

#include <iostream>

using namespace std;

SSH::SSH(const string& ip, const string& pass) :
	ip_(ip), pass_(pass) {
	connected_ = false;
}

void SSH::disconnect() {
	if (!connected_)
		return;
		
	ssh_free(session_);
	ssh_disconnect(session_);
	
	cout << "Debug: disconnected from " << ip_ << endl;
}

bool SSH::connect() {
	session_ = ssh_new();
	
	if (session_ == NULL) {
		cout << "Error: could not create SSH session\n";
		
		ssh_free(session_);
		return false;
	}
	
	ssh_options_set(session_, SSH_OPTIONS_HOST, ip_.c_str());
	ssh_options_set(session_, SSH_OPTIONS_USER, "root");
	
	if (ssh_connect(session_) != SSH_OK) {
		cout << "Error: could not connect to " << ip_ << " code: " << ssh_get_error(session_) << endl;
		
		ssh_free(session_);
		return false;
	}
	
	if (ssh_userauth_password(session_, NULL, pass_.c_str()) != SSH_AUTH_SUCCESS) {
		cout << "Error: wrong password for " << ip_ << endl;
		
		disconnect();
		return false;
	}
	
	connected_ = true;
	
	//cout << "Debug: connected to " << ip_ << endl;
	
	return true;
}

bool SSH::command(const string& command) {
	if (!connected_) {
		cout << "Error: could not execute command, we're not connected\n";
		
		return false;
	}
	
	ssh_channel channel = ssh_channel_new(session_);
	
	if (channel == NULL) {
		cout << "Error: could not open channel\n";
		
		return false;
	}
	
	if (ssh_channel_open_session(channel) != SSH_OK) {
		cout << "Error: could not open channel (2)\n";
		
		ssh_channel_free(channel);
		return false;
	}
	
	if (ssh_channel_request_exec(channel, command.c_str()) != SSH_OK) {
		cout << "Error: could not execute command\n";
		
		ssh_channel_close(channel);
		ssh_channel_free(channel);
		return false;
	}
	
	ssh_channel_send_eof(channel);
	ssh_channel_close(channel);
	ssh_channel_free(channel);
	
	return true;
}

bool SSH::operator==(const string& ip) {
	return ip == ip_;
}