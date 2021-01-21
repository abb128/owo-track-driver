#pragma once

#include "Network.h"
#include "RemoteTracker.h"
#include <vector>

class InfoServer {
private:
	static const int INFO_PORT = 35903;
	
	UDPSocket Socket;
	std::vector<RemoteTracker *> list;

	char* buff;
	static const int MAX_BUFF_SIZE = 64;

	bool respond_to_all_requests();

	std::string response_info = "";
public:
	InfoServer();

	void add_tracker(RemoteTracker *tracker);
	void tick();
};