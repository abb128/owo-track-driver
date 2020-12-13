#pragma once

#include "abstract_ipc.h"


#define _WINSOCKAPI_
// oh my god

#include <Windows.h>

#include <queue>
#include <mutex>
#include <string>

class Win32IPC : public AbstractIPC {
private:
	static constexpr unsigned int BUFFSIZE = 1024;

	HANDLE hSlot = INVALID_HANDLE_VALUE;

	std::queue<IPCData> received_buffer;

	bool server_mode = false;

	bool read_slot(int max_num_msgs);

	bool init_mailslot();

	std::string name;

	bool should_continue_running = true;

public:
	Win32IPC(bool is_server, std::string pipe_name); // TODO could be seperated to Win32IPCServer and Win32IPCClient

	void init();
	void destroy();

	bool is_connected();
	bool is_data_waiting();

	IPCData get_data();
	void put_data(IPCData data);
};