#pragma once

#include <string>
#include <tchar.h>

struct IPCData {
	void* buffer;
	unsigned long data_length;

	void free() {
		std::free(buffer);
	}
};

class AbstractIPC {
public:
	// AbstractIPC(std::string pipeName);

	virtual bool is_connected() = 0; // returns true is if connected to server/client
	virtual bool is_data_waiting() = 0;
	
	virtual IPCData get_data() = 0;
	virtual void put_data(IPCData data) = 0;
};