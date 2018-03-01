#ifndef SSH_H
#define SSH_H

#include <string>

// Fuck the C++ wrapper
#include <libssh/libssh.h>

class SSH {
public:
	SSH(const std::string& ip, const std::string& pass);
	
	bool connect();
	void disconnect();
	bool command(const std::string& command);
	
	bool operator==(const std::string& ip);
	
private:
	std::string ip_;
	std::string pass_;
	
	bool connected_;
	ssh_session session_;
};

#endif