#include "DeviceProvider.h"

#include <thread>
#include <windows.h>
#include "UDPDeviceQuatServer.h"

#include "HipMoveController.h"

int DeviceProvider::add_tracker(const int& port) {
	if (ports_taken.count(port) > 0) {
		DriverLog("PORT %d IS ALREADY TAKEN!!!", port);
		return -1;
	}

	unsigned int id = trackers.size();

	RemoteTrackerSettings defaults;

	defaults.anchor_device_id = 0;
	defaults.offset_global = Vector3(0.0, 0.0, 0.0);
	defaults.offset_local_device = Vector3(0.0, 0.0, 0.0);
	defaults.offset_local_tracker = Vector3(0.0, -0.73, 0.0);

	defaults.local_rot_euler = Vector3(0.0, 0.0, 0.0);
	defaults.global_rot_euler = Vector3(0.0, 0.0, 0.0);

	defaults.yaw_offset = 0.0;

	defaults.should_predict_position = false;

	UDPDeviceQuatServer* server = new UDPDeviceQuatServer(port);
	RemoteTracker* tracker = new RemoteTracker(server, id, defaults);
	vr::VRServerDriverHost()->TrackedDeviceAdded(tracker->GetSerialNumber(), vr::TrackedDeviceClass_GenericTracker, tracker);

	devices.push_back(tracker);
	trackers.push_back(tracker);

	ports_taken.insert({ port, true });

	srv.add_tracker(tracker);

	return (int)id;
}

EVRInitError DeviceProvider::Init(vr::IVRDriverContext* pDriverContext) {
	VR_INIT_SERVER_DRIVER_CONTEXT(pDriverContext);
	InitDriverLog(vr::VRDriverLog());
	poses = (TrackedDevicePose_t*)malloc(sizeof(TrackedDevicePose_t) * k_unMaxTrackedDeviceCount);

	to_overlay.init();
	from_overlay.init();

	return VRInitError_None;
}

void DeviceProvider::Cleanup() {
	CleanupDriverLog();
	for (auto v : trackers) {
		delete ((RemoteTracker*)(v));
	}
	from_overlay.destroy();
	to_overlay.destroy();
}


constexpr unsigned int CURR_VERSION = 8;

owoEvent DeviceProvider::handle_event(const owoEvent& ev) {
	switch (ev.type) {
		case GET_VERSION:
			return { .type = VERSION_RECEIVED, .index = CURR_VERSION };
		case GET_TRACKERS_LEN:
			return { .type = TRACKERS_LEN_RECEIVED, .index = (unsigned int)trackers.size() };
		case GET_TRACKER: {
			if (trackers.size() <= ev.index) return noneEvent;
			RemoteTracker* tracker = trackers[ev.index];
			
			if (tracker == nullptr) {
				return noneEvent;
			}

			return { .type = TRACKER_RECEIVED, .tracker = owoTracker { tracker != nullptr, ev.index, tracker->port_no } };
		}
		case BYPASS_DELAY: {
			should_bypass_waiting = true;
			return noneEvent;
		}

		case SET_TRACKER_SETTING:
		case GET_TRACKER_SETTING: {
			if (trackers.size() <= ev.trackerSetting.tracker_id) return noneEvent;
			RemoteTracker* tracker = trackers[ev.trackerSetting.tracker_id];
			if (tracker == nullptr) return noneEvent;
			return tracker->process_request(ev);
		}

		case CREATE_TRACKER: {
			unsigned int id = add_tracker(ev.trackerCreation.port);
			return { .type = TRACKER_CREATED, .index = id };
		}

		case DESTROY_TRACKER: {
			if (trackers.size() <= ev.index) return noneEvent;
			RemoteTracker* tracker = trackers[ev.index];
			if (tracker == nullptr) return noneEvent;

			tracker->send_invalid_pose();
			trackers[ev.index] = nullptr;

			return noneEvent;
		}
	}

	return noneEvent;
}

void DeviceProvider::tick_ipc() {
	while (from_overlay.is_data_waiting()) {
		IPCData data = from_overlay.get_data();

		if (data.data_length != sizeof(owoEvent)) {
			DriverLog("ipc tick that wanst supposed to happen");
			DriverLog(std::to_string(data.data_length).c_str());
			DriverLog(std::to_string(sizeof(owoEvent)).c_str());
			data.free();
			return;
		}

		owoEvent ev;
		memcpy(&ev, data.buffer, data.data_length);
		data.free();

		owoEvent response = handle_event(ev);
		if (response.type != INVALID_EVENT) {
			IPCData new_data = { (void *)&response, sizeof(owoEvent) };
			to_overlay.put_data(new_data);
		}
	}
}


void DeviceProvider::RunFrame() {
	tick_ipc();

	VRServerDriverHost()->GetRawTrackedDevicePoses(0, poses, k_unMaxTrackedDeviceCount);

	for (auto v : devices) {
		if (v == nullptr) continue;
		v->RunFrame(poses);
	}

	vr::VREvent_t vrEvent;
	while (vr::VRServerDriverHost()->PollNextEvent(&vrEvent, sizeof(vrEvent))) {
		for (auto v : devices) {
			if (v == nullptr) continue;
			v->ProcessEvent(vrEvent);
		}
	}

	srv.tick();
}