#include "Connections.h"
#include "SSH.h"

#include <iostream>

#define ERROR(...)	do { fprintf(stderr, "Error: "); fprintf(stderr, __VA_ARGS__); fprintf(stderr, "\n"); exit(1); } while(0)

using namespace std;

int main(int argc, char** argv) {
	Connections master;
	
	cout << "Hello user, here we're adjusting the affordable speakers! ;)\n";
	
	vector<string> ips(argv + 1, argv + argc);
	vector<string> equalizers;
	
	for (auto& ip : ips) {
		// dspd -e 0,0,0,0,0,0,0,0,0
		equalizers.push_back("dspd -e 0,0,0,-10,-10,-10,-10,-10,-10");
		cout << "IP: " << ip << endl;
	}
	
	master.connect(ips, "pass");
	master.command(ips, equalizers);
	
	return 0;
}