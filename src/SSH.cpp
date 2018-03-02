#include "SSH.h"

#include <iostream>
#include <fstream>
#include <vector>

#include <sys/stat.h>

#define ERROR(...)	do { fprintf(stderr, "Error: "); fprintf(stderr, __VA_ARGS__); fprintf(stderr, "\n"); exit(1); } while(0)

using namespace std;

SSH::SSH(const string& ip, const string& pass) :
	ip_(ip), pass_(pass) {
	connected_ = false;
}

void SSH::disconnect() {
	if (!connected_)
		return;
		
	ssh_disconnect(session_);
	ssh_free(session_);
}

static vector<string> splitString(const string& input, char split) {
	vector<string> splitted;
	size_t last_split = 0;
	
	for (size_t i = 0; i < input.length(); i++) {
		if (input.at(i) == split) {
			splitted.push_back(input.substr(last_split, i - last_split));
			last_split = i + 1;
		}
	}
	
	// Don't forget last token
	if (last_split != input.length()) {
		splitted.push_back(input.substr(last_split, input.length() - last_split));
	}
	
	return splitted;
}

static string getFilenameFromPath(const string& path) {
	for (int i = path.length() - 1; i >= 0; i--)
		if (path.at(i) == '/')
			return path.substr(i + 1);
			
	return "";		
}

static size_t getFileSize(string& filename) {
	ifstream file(filename, ios::binary | ios::ate);
	
	if (!file.is_open()) {
		cout << "Warning: could not open file to get file size\n";
		
		return 0;
	}
	
	size_t size = file.tellg();
	file.close();
	
	return size;
}

bool SSH::transferRemote(const string& from, const string& to) {
	if (!connected_) {
		cout << "Warning: can't read from SCP without an active SSH connection\n";
		
		return false;
	}
	
	ssh_scp scp = ssh_scp_new(session_, SSH_SCP_WRITE | SSH_SCP_RECURSIVE, to.c_str());
	
	if (scp == NULL) {
		cout << "Error: could not create SCP\n";
		
		return false;
	}
	
	if (ssh_scp_init(scp) != SSH_OK) {
		cout << "Error: could not initialize SCP session\n";
		
		ssh_scp_free(scp);
		return false;
	}

	vector<string> files = splitString(from, ' ');
	
	for (auto& filename : files) {
		ifstream file(filename);
		
		if (!file.is_open()) {
			cout << "Warning: could not write file to remote host (" << filename << ")\n";
			
			ssh_scp_close(scp);
			ssh_scp_free(scp);
			return false;
		}
		
		const size_t FILE_BUFFER_SIZE = 16384;
		char* file_buffer = new char[FILE_BUFFER_SIZE];
		
		size_t file_size = getFileSize(filename);
		string remote_file = getFilenameFromPath(filename);

		if (ssh_scp_push_file(scp, remote_file.c_str(), file_size, S_IRWXU) != SSH_OK) {
			cout << "Warning: could not push file to remote host with RWX\n";
			
			ssh_scp_close(scp);
			ssh_scp_free(scp);
			delete[] file_buffer;
			return false;
		}
		
		size_t left = file_size;
		
		while (true) {
			size_t read_amount = left > FILE_BUFFER_SIZE ? FILE_BUFFER_SIZE : left;
			file.read(file_buffer, read_amount);
			
			if (!file)
				ERROR("error reading file\n");
				
			int wrote = ssh_scp_write(scp, file_buffer, read_amount);
			
			if (wrote != SSH_OK) {
				cout << "Warning: could not write data to remote file\n";
				
				ssh_scp_close(scp);
				ssh_scp_free(scp);
				delete[] file_buffer;
				return false;
			}
			
			left -= read_amount;
			
			if (left == 0)
				break;
		}
		
		file.close();
		
		delete[] file_buffer;
		
		//cout << "Wrote local file " << filename << " to remote file " << remote_file << endl;
	}
	
	ssh_scp_close(scp);
	ssh_scp_free(scp);
	
	return true;
}

bool SSH::transferLocal(const string& from, const string& to) {
	if (!connected_) {
		cout << "Warning: can't read from SCP without an active SSH connection\n";
		
		return false;
	}
		
	ssh_scp scp = ssh_scp_new(session_, SSH_SCP_READ | SSH_SCP_RECURSIVE, from.c_str());
	
	if (scp == NULL) {
		cout << "Error: could not create SCP\n";
		
		return false;
	}
	
	if (ssh_scp_init(scp) != SSH_OK) {
		cout << "Error: could not create reading SCP session\n";
		
		ssh_scp_free(scp);
		return false;
	}
	
	bool succeeded = true;
	
	do {
		bool end = false;
		
		switch (ssh_scp_pull_request(scp)) {
			case SSH_SCP_REQUEST_NEWFILE: {
				size_t size = ssh_scp_request_get_size(scp);
				string filename = ssh_scp_request_get_filename(scp);
				const size_t FILE_BUFFER_SIZE = 16384;
				char* file_buffer = new char[FILE_BUFFER_SIZE];
				
				//cout << "Downloading file: " << filename << " with size " << size << endl;
				
				ofstream file(to + "/" + filename);
				
				if (!file.is_open()) {
					cout << "Warning: could not open file for writing local SCP\n";
					
					succeeded = false;
					end = true;
					break;
				}
				
				size_t left = size;
				ssh_scp_accept_request(scp);
				
				while (true) {
					int read = ssh_scp_read(scp, file_buffer, left > FILE_BUFFER_SIZE ? FILE_BUFFER_SIZE : left);
					
					if (read == SSH_ERROR) {
						cout << "Error reading SCP\n";
						
						succeeded = false;
						end = true;
						break;
					}
					
					file.write(file_buffer, read);
					
					left -= read;
					
					if (left == 0)
						break;
				}
				
				file.close();
				
				break;
			}
			
			case SSH_ERROR: {
				cout << "Error: SCP encountered an error (" << ssh_get_error(session_) << ")\n";
				
				succeeded = false;
				end = true;
				break;
			}
			
			case SSH_SCP_REQUEST_WARNING: {
				cout << "Warning: received a warning from SCP (" << ssh_scp_request_get_warning(scp) << ")\n";
				
				break;
			}
			
			case SSH_SCP_REQUEST_EOF: {
				end = true;
				break;
			}
		}
		
		if (end)
			break;
	} while (true);
	
	ssh_scp_close(scp);
    ssh_scp_free(scp);
	
	return succeeded;
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
	ssh_options_set(session_, SSH_OPTIONS_STRICTHOSTKEYCHECK, 0 /* Do not ask for fingerprint approval */);
	
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