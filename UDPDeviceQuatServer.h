#pragma once

#include "NetworkedDeviceQuatServer.h"
#include "Network.h"

#include "ByteBuffer.h"
using namespace bb;

class UDPDeviceQuatServer : public NetworkedDeviceQuatServer {
private:
	int portno;
	WSASession Session;
	UDPSocket Socket;

	sockaddr_in client;

	void send_heartbeat();

	char* buffer;

	bool more_data_exists__read();

	unsigned long long last_contact_time = 0;

	bool connectionIsDead = false;

	int hb_accum;

	void send_bytebuffer(ByteBuffer& b);

public:
	UDPDeviceQuatServer(int portno_v);

	void startListening();
	void tick();

	bool isConnectionAlive();

	void buzz(float duration_s, float frequency, float amplitude);
};
