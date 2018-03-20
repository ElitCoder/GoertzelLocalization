#ifndef SSHMASTER_H
#define SSHMASTER_H

#include "SSH.h"

#include <vector>
#include <mutex>

enum {
	SETTING_USE_ACTUAL_FILENAME,
	SETTING_ENABLE_SSH_OUTPUT,
	SETTING_ENABLE_SSH_OUTPUT_VECTOR_STYLE,
	SETTING_MAX
};

class SSHMaster {
public:
	SSHMaster();
	~SSHMaster();
	
	bool connect(const std::string& ip, const std::string& pass);
	bool connect(const std::string& ip, const std::string& user, const std::string& pass);
	bool connect(const std::vector<std::string>& ips, const std::string& pass);
	bool connect(const std::vector<std::string>& ips, const std::vector<std::string>& users, const std::vector<std::string>& passwords);
	bool command(std::vector<std::string>& ips, std::vector<std::string>& commands);
	std::vector<std::pair<std::string, std::vector<std::string>>> command(const std::vector<std::string>& ips, const std::vector<std::string>& commands);
	bool transferLocal(std::vector<std::string>& ips, std::vector<std::string>& from, std::vector<std::string>& to, bool threading);
	bool transferRemote(std::vector<std::string>& ips, std::vector<std::string>& from, std::vector<std::string>& to);
	
	void setSetting(int setting, bool value);
	bool getSetting(int setting);
	
	void setThreadedConnectionStatus(bool status);
	SSH& getSession(const std::string& ip, bool threading);
	
private:
	bool threaded_connections_result_;
	std::mutex threaded_connections_mutex_;
	
	std::vector<SSH> connections_;
	std::vector<bool> settings_;
};

#endif