#ifndef CONNECTIONS_H
#define CONNECTIONS_H

#include "SSH.h"

#include <vector>
#include <mutex>

class Connections {
public:
	explicit Connections();
	~Connections();
	
	bool connect(const std::string& ip, const std::string& pass);
	bool connect(const std::vector<std::string>& ips, const std::string& pass);
	bool command(const std::string& ip, const std::string& command);
	
	void setThreadedConnectionStatus(bool status);
	
private:
	bool threaded_connections_result_;
	std::mutex threaded_connections_mutex_;
	
	std::vector<SSH> connections_;
};

#endif