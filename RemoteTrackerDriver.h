#pragma once

#include "RemoteTracker.h"
#include <openvr_driver.h>
#include <map>

#include "abstract_ipc.h"
#include "win32ipc.h"

#include "owoIPC.h"

class RemoteTrackerDriver : public IServerTrackedDeviceProvider {
private:
	int add_tracker(const int& port);
	std::vector<RemoteTracker*> trackers;
	std::map<int, bool> ports_taken;
	TrackedDevicePose_t* poses;

	Win32IPC to_overlay = Win32IPC(false, "\\\\.\\mailslot\\owoTrack-driver-pipe-to-overlay");
	Win32IPC from_overlay = Win32IPC(true, "\\\\.\\mailslot\\owoTrack-driver-pipe-from-overlay");

	owoEvent handle_event(const owoEvent& ev);
	void tick_ipc();

	bool should_bypass_waiting = false;

public:
	virtual EVRInitError Init(vr::IVRDriverContext* pDriverContext);
	virtual void Cleanup();
	virtual const char* const* GetInterfaceVersions() { return vr::k_InterfaceVersions; }
	virtual void RunFrame();
	virtual bool ShouldBlockStandbyMode() { return false; }
	virtual void EnterStandby() {}
	virtual void LeaveStandby() {}
};