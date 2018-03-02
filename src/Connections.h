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
	bool command(std::vector<std::string>& ips, std::vector<std::string>& commands);
	bool transferLocal(std::vector<std::string>& ips, std::vector<std::string>& from, std::vector<std::string>& to, bool threading);
	bool transferRemote(std::vector<std::string>& ips, std::vector<std::string>& from, std::vector<std::string>& to);
	
	void setThreadedConnectionStatus(bool status);
	SSH& getSession(const std::string& ip, bool threading);
	
private:
	bool threaded_connections_result_;
	std::mutex threaded_connections_mutex_;
	
	std::vector<SSH> connections_;
};

#endif