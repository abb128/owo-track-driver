#include "InfoServer.h"

#include "driverlog.h"

bool InfoServer::respond_to_all_requests(){
	sockaddr_in addr;
	bool is_recv = Socket.RecvFrom(buff, MAX_BUFF_SIZE, reinterpret_cast<SOCKADDR*>(&addr));
	if (!is_recv) return false;

	if (strcmp(buff, "DISCOVERY\0") == 0) {
		Socket.SendTo(addr, response_info.c_str(), response_info.length());
	}
}

InfoServer::InfoServer(){
	buff = (char *)malloc(MAX_BUFF_SIZE);
	Socket.Bind(INFO_PORT);
}

void InfoServer::add_tracker(RemoteTracker *tracker){
	list.push_back(tracker);
	

	response_info = "";
	for (RemoteTracker *trk : list) {
		response_info = response_info + std::to_string(trk->port_no) + ":" + trk->get_description() + "\n";
	}
}

void InfoServer::tick(){
	while (respond_to_all_requests()) {};
}
