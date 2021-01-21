#include "InfoServer.h"

#include "driverlog.h"

bool InfoServer::respond_to_all_requests(){
	sockaddr_in addr;
	bool is_recv = Socket.RecvFrom(buff, MAX_BUFF_SIZE, reinterpret_cast<SOCKADDR*>(&addr));
	if (!is_recv) return false;

	DriverLog("Received request %s", buff);

	if (strcmp(buff, "DISCOVERY\0") == 0) {
		DriverLog("Send response %s", response_info.c_str());
		Socket.SendTo(addr, response_info.c_str(), response_info.length());
	}
}

InfoServer::InfoServer(){
	buff = (char *)malloc(MAX_BUFF_SIZE);
	Socket.Bind(INFO_PORT);

	// decided not to use multicast
	/*
	for (int i = 0; i < 16; i++) {
		std::string beginning = "0.0.0.";
		struct ip_mreq req;
		req.imr_multiaddr.s_addr = inet_addr("234.35.90.3");
		req.imr_interface.s_addr = inet_addr((beginning + std::to_string(i)).c_str());


		setsockopt(
			Socket.sock,
			IPPROTO_IP,
			IP_ADD_MEMBERSHIP,
			(char*)&req,
			sizeof(req)
		);
	}
	*/
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
