#pragma once

struct owoTrackerCreationData {
	unsigned int port;
};

struct owoTracker {
	bool exists;
	unsigned int ovrDeviceIdx;
	unsigned int port_no;
};

struct owoEventVector {
	double x;
	double y;
	double z;
};

enum owoTrackerSettingType {
	ANCHOR_DEVICE_ID,		// index

	OFFSET_GLOBAL,			// vector
	OFFSET_LOCAL_TO_DEVICE,	// vector
	OFFSET_LOCAL_TO_TRACKER,// vector

	OFFSET_ROT_GLOBAL, // vector
	OFFSET_ROT_LOCAL,  // vector

	YAW_VALUE,				// double_v

	PREDICT_POSITION,		// bool_v
	PREDICT_POSITION_STRENGTH, // double_v

	IS_CALIBRATING,			// bool_v
	IS_CONN_ALIVE,			// bool_v, read-only

	CALIBRATING_DOWN,		// bool_v
	HIP_MOVE,				// bool_v
	HIP_MOVE_VECTOR			// vector
};

struct owoEventTrackerSetting {
	unsigned int tracker_id;
	owoTrackerSettingType type;
	union {
		unsigned int index;
		owoEventVector vector;
		double double_v;
		bool bool_v;
	};
};


enum owoEventType {
	INVALID_EVENT,

	GET_VERSION,
	VERSION_RECEIVED, // the version - index

	GET_TRACKERS_LEN,
	TRACKERS_LEN_RECEIVED, // number of trackers that exist - index

	GET_TRACKER, // tracker index - index
	TRACKER_RECEIVED, // tracker - tracker

	BYPASS_DELAY,

	SET_TRACKER_SETTING, // trackerSetting
	GET_TRACKER_SETTING,  // trackerSetting, v is nothing
	TRACKER_SETTING_RECEIVED, // trackerSetting

	CREATE_TRACKER, // trackerCreation
	TRACKER_CREATED, // new tracker index - index

	DESTROY_TRACKER // tracker index - index
};

struct owoEvent {
	owoEventType type;
	union {
		owoTracker tracker;
		owoTrackerCreationData trackerCreation;
		owoEventTrackerSetting trackerSetting;
		unsigned int index;
	};
};

constexpr owoEvent noneEvent = { INVALID_EVENT, 0 };


template<typename T>
inline T& get_ref_from_setting_event(owoEventTrackerSetting& ev) {
	switch (ev.type) {
	case ANCHOR_DEVICE_ID:
		return (T&)ev.index;

	case YAW_VALUE:
	case PREDICT_POSITION_STRENGTH:
		return (T&)ev.double_v;

	case PREDICT_POSITION:
	case CALIBRATING_DOWN:
	case IS_CALIBRATING:
	case IS_CONN_ALIVE:
	case HIP_MOVE:
		return (T&)ev.bool_v;

	case OFFSET_GLOBAL:
	case OFFSET_LOCAL_TO_DEVICE:
	case OFFSET_LOCAL_TO_TRACKER:
	case OFFSET_ROT_GLOBAL:
	case OFFSET_ROT_LOCAL:
	case HIP_MOVE_VECTOR:
		return (T&)ev.vector;
	}
}