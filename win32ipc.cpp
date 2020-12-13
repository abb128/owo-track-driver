#include "logging.h"
#include "win32ipc.h"


// holy hell why is win32 api so bad

Win32IPC::Win32IPC(bool is_server, std::string pipe_name) {
	server_mode = is_server;
	name = pipe_name;
}

void Win32IPC::init() {
	LOG_FUNC("IPC init");
	init_mailslot();
}

void Win32IPC::destroy() {
	should_continue_running = false;

	if (hSlot != INVALID_HANDLE_VALUE) {
		CloseHandle(hSlot);
	}

	for (int i = 0; i < received_buffer.size(); i++) {
		IPCData dat = received_buffer.front();
		received_buffer.pop();
		dat.free();
	}
}

bool Win32IPC::is_connected() {
	return true;
}

bool Win32IPC::is_data_waiting() {
	return read_slot(0) == true;
}

IPCData Win32IPC::get_data() {
	if (received_buffer.size() == 0)
		read_slot(1);

	IPCData dat = received_buffer.front();
	received_buffer.pop();
	return dat;
}

void Win32IPC::put_data(IPCData data) {
	if (server_mode) {
		LOG_FUNC("Cannot put data from server!");
		return;
	}
	// probably shouldnt silently fail
	if (hSlot == INVALID_HANDLE_VALUE) return;

	DWORD b_written;
	bool success = WriteFile(hSlot, data.buffer, data.data_length, &b_written, NULL);
	if (!success) {
		LOG_FUNC("IPC Write failed");
		if (GetLastError() == 38) {
			// procss restarted? retry
			init_mailslot();
			return put_data(data);
		}
		LOG_FUNC(std::to_string(GetLastError()).c_str());
		return;
	}
}

bool Win32IPC::read_slot(int max_num_msgs) {
	if (!server_mode) {
		LOG_FUNC("Cannot read data from client!");
		LOG_FUNC(name.c_str());
		return false;
	}
	if (hSlot == INVALID_HANDLE_VALUE) return false;

	DWORD message_size, num_msgs;

	bool result = GetMailslotInfo(
		hSlot,
		NULL,
		&message_size,
		&num_msgs,
		NULL
	);

	if (!result) {
		LOG_FUNC("GetMailSlot failed");
		return false;
	}

	if (message_size == MAILSLOT_NO_MESSAGE) {
		return false;
	}

	HANDLE hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	if (hEvent == NULL) return false;

	OVERLAPPED ov;
	ov.Offset = 0;
	ov.OffsetHigh = 0;
	ov.hEvent = hEvent;


	for (int i = 0; i < max_num_msgs; i++) {
		if (num_msgs == 0) return false;

		void* buff = malloc(BUFFSIZE);

		DWORD b_read;
		result = ReadFile(hSlot, buff, BUFFSIZE, &b_read, &ov);

		if (!result) {
			LOG_FUNC("IPC Read failed");
			LOG_FUNC(std::to_string(GetLastError()).c_str());
			return false;
		}


		bool result = GetMailslotInfo(
			hSlot,
			NULL,
			&message_size,
			&num_msgs,
			NULL
		);
		if (!result) {
			LOG_FUNC("GetMailSlot failed");
			return false;
		}

		received_buffer.push({ buff, b_read });
	}
	CloseHandle(hEvent);

	return (num_msgs > 0);
}

bool Win32IPC::init_mailslot() {
	if (server_mode) {
		hSlot = CreateMailslotA(
			name.c_str(),
			0,
			MAILSLOT_WAIT_FOREVER,
			NULL
		);

		if (hSlot == INVALID_HANDLE_VALUE) {
			LOG_FUNC("IPC CreateMailslotA failed!!!");
			return false;
		}
	}
	else {
		hSlot = CreateFileA(
			name.c_str(),
			GENERIC_WRITE,
			FILE_SHARE_READ,
			NULL,
			OPEN_EXISTING,
			FILE_ATTRIBUTE_NORMAL,
			NULL
		);

		if (hSlot == INVALID_HANDLE_VALUE) {
			if (GetLastError() == 2) { // file not found, probably one of the apps hasnt yet loaded
				std::thread* delay = new std::thread([&] {
					std::this_thread::sleep_for(std::chrono::milliseconds(1000));
					init_mailslot();
				});
			}
			LOG_FUNC("IPC CreateFile failed!!!");
			LOG_FUNC(name.c_str());
			LOG_FUNC(std::to_string(GetLastError()).c_str());
			return false;
		}
	}

	return true;
}
