#include "RemoteTracker.h"

#include <openvr_driver.h>
#include <system_error>


#if defined( _WINDOWS )
#include <windows.h>
#endif

#include "quaternion.h"
#include "basis.h"

// TODO for convert_chars
#include "NetworkedDeviceQuatServer.h"

using namespace vr;

RemoteTracker::RemoteTracker(DeviceQuatServer *server, const int& id_v, RemoteTrackerSettings settings_v) : dataserver(server), settings(settings_v), id(id_v) {
	m_unObjectId = vr::k_unTrackedDeviceIndexInvalid;
	m_ulPropertyContainer = vr::k_ulInvalidPropertyContainer;

	m_sSerialNumber = "OWO_TRACKER_" + std::to_string(id);
	m_sModelNumber = "OwoTracker_" + std::to_string(id);

	port_no = dataserver->get_port();
}

RemoteTracker::~RemoteTracker()
{
	delete dataserver;
}

EVRInitError RemoteTracker::Activate(vr::TrackedDeviceIndex_t unObjectId)
{
	vr::VRSettings()->SetString(k_pch_Trackers_Section, ("/devices/htc/vive_tracker" + m_sSerialNumber).c_str(), "TrackerRole_Waist");



	m_unObjectId = unObjectId;
	m_ulPropertyContainer = vr::VRProperties()->TrackedDeviceToPropertyContainer(m_unObjectId);



	vr::VRProperties()->SetStringProperty(m_ulPropertyContainer, Prop_ModelNumber_String, "Vive Tracker Pro MV");
	vr::VRProperties()->SetStringProperty(m_ulPropertyContainer, Prop_RenderModelName_String, "{htc}vr_tracker_vive_1_0");

	vr::VRProperties()->SetBoolProperty(m_ulPropertyContainer, Prop_NeverTracked_Bool, false);


	vr::VRProperties()->SetInt32Property(m_ulPropertyContainer, Prop_DeviceClass_Int32, TrackedDeviceClass_GenericTracker);

	std::string l_registeredType("htc/vive_tracker");
	l_registeredType.append(m_sSerialNumber);
	vr::VRProperties()->SetStringProperty(m_ulPropertyContainer, Prop_RegisteredDeviceType_String, l_registeredType.c_str());
	vr::VRProperties()->SetStringProperty(m_ulPropertyContainer, Prop_InputProfilePath_String, "{owoTrack}/input/remote_profile.json");

	vr::VRProperties()->SetBoolProperty(m_ulPropertyContainer, Prop_Identifiable_Bool, true);
	vr::VRProperties()->SetBoolProperty(m_ulPropertyContainer, Prop_Firmware_RemindUpdate_Bool, false);
	vr::VRProperties()->SetInt32Property(m_ulPropertyContainer, Prop_ControllerRoleHint_Int32, TrackedControllerRole_Invalid); // should this be OptOut? see IsRoleAllowedAsHand
	
	vr::VRProperties()->SetStringProperty(m_ulPropertyContainer, Prop_ControllerType_String, "vive_tracker_waist");
	
	vr::VRProperties()->SetInt32Property(m_ulPropertyContainer, Prop_ControllerHandSelectionPriority_Int32, -1);

	vr::VRProperties()->SetBoolProperty(m_ulPropertyContainer, Prop_HasDisplayComponent_Bool, false);
	vr::VRProperties()->SetBoolProperty(m_ulPropertyContainer, Prop_HasCameraComponent_Bool, false);
	vr::VRProperties()->SetBoolProperty(m_ulPropertyContainer, Prop_HasDriverDirectModeComponent_Bool, false);
	vr::VRProperties()->SetBoolProperty(m_ulPropertyContainer, Prop_HasVirtualDisplayComponent_Bool, false);

	vr::VRProperties()->SetUint64Property(m_ulPropertyContainer, vr::Prop_CurrentUniverseId_Uint64, 2);

	vr::VRDriverInput()->CreateHapticComponent(m_ulPropertyContainer, "/output/haptic", &haptic);


	try {
		dataserver->startListening();
	}
	catch (std::system_error& e) {
		DriverLog("*** LISTEN FAILED ***");
		DriverLog(e.what());
		return VRInitError_Driver_Failed;
	}

	activated = true;

	return VRInitError_None;
}

void RemoteTracker::Deactivate()
{
	m_unObjectId = vr::k_unTrackedDeviceIndexInvalid;
}

void RemoteTracker::EnterStandby(){}

void* RemoteTracker::GetComponent(const char* pchComponentNameAndVersion)
{
	// override this to add a component to a driver
	return NULL;
}

void RemoteTracker::PowerOff(){}

void RemoteTracker::DebugRequest(const char* pchRequest, char* pchResponseBuffer, uint32_t unResponseBufferSize){}

DriverPose_t RemoteTracker::GetPose()
{
	DriverPose_t pose = { 0 };
	pose.poseIsValid = false;
	pose.result = TrackingResult_Calibrating_OutOfRange;
	pose.deviceIsConnected = false;

	pose.vecPosition[0] = 9999999;
	pose.vecPosition[1] = 9999999;
	pose.vecPosition[2] = 9999999;

	pose.vecVelocity[0] = 0.0;
	pose.vecVelocity[1] = 0.0;
	pose.vecVelocity[2] = 0.0;

	pose.qWorldFromDriverRotation = quaternion::init(0, 0, 0, 1);
	pose.qDriverFromHeadRotation = quaternion::init(0, 0, 0, 1);

	return pose;
}


void RemoteTracker::send_invalid_pose() {
	DriverPose_t pose = GetPose();

	VRServerDriverHost()->TrackedDevicePoseUpdated(m_unObjectId, pose, sizeof(pose));
}


template<typename T>
inline owoEvent RemoteTracker::give_value(T local_val, owoEvent ev) {
	owoEvent ret_event = {};
	ret_event.type = TRACKER_SETTING_RECEIVED;

	owoEventTrackerSetting s = {};
	s.type = ev.trackerSetting.type;
	s.tracker_id = id;

	get_ref_from_setting_event<T>(s) = local_val;


	ret_event.trackerSetting = s;

	return ret_event;
}


template<typename T>
inline owoEvent RemoteTracker::set_setting_or_give_value(T& local_val, owoEvent ev){
	if (ev.type == SET_TRACKER_SETTING) {
		local_val = get_ref_from_setting_event<T>(ev.trackerSetting);
		return noneEvent;
	} else {
		return give_value(local_val, ev);
	}
}


owoEvent RemoteTracker::handle_controller(owoEvent ev) {
	if (ev.type == SET_TRACKER_SETTING) {
		bool tgt = get_ref_from_setting_event<bool>(ev.trackerSetting);
		if (!associated_controller) {
			if (!tgt) return noneEvent;
			associated_controller = new HipMoveController(this);
			vr::VRServerDriverHost()->TrackedDeviceAdded(associated_controller->GetSerialNumber(), vr::TrackedDeviceClass_Controller, associated_controller);
		}

		associated_controller->enabled = tgt;

		return noneEvent;
	} else {
		if (!associated_controller) return give_value(false, ev);
		return give_value(associated_controller->enabled, ev);
	}
}

inline owoEvent RemoteTracker::handle_vector(Vector3& local_val, owoEvent ev) {
	owoEventVector tmp = { local_val.x, local_val.y, local_val.z };
	owoEvent result = set_setting_or_give_value(tmp, ev);

	if (result.type == INVALID_EVENT) {
		local_val.x = tmp.x;
		local_val.y = tmp.y;
		local_val.z = tmp.z;
	}

	return result;
}

owoEvent RemoteTracker::process_request(owoEvent ev){
	owoEventTrackerSetting s = ev.trackerSetting;
	switch (s.type) {
		case ANCHOR_DEVICE_ID:
			return set_setting_or_give_value(settings.anchor_device_id, ev);
		case YAW_VALUE:
			return set_setting_or_give_value(settings.yaw_offset, ev);
		case PREDICT_POSITION_STRENGTH:
			return set_setting_or_give_value(settings.position_prediction_strength, ev);
		case PREDICT_POSITION:
			return set_setting_or_give_value(settings.should_predict_position, ev);
		case IS_CALIBRATING:
			return set_setting_or_give_value(is_calibrating, ev);
		case CALIBRATING_DOWN:
			return set_setting_or_give_value(is_down_calibrating, ev);
		case IS_CONN_ALIVE: 
			return give_value(dataserver->isConnectionAlive(), ev);
		case OFFSET_GLOBAL:
			return handle_vector(settings.offset_global, ev);
		case OFFSET_LOCAL_TO_DEVICE:
			return handle_vector(settings.offset_local_device, ev);
		case OFFSET_LOCAL_TO_TRACKER:
			return handle_vector(settings.offset_local_tracker, ev);
		case OFFSET_ROT_GLOBAL:
			return handle_vector(settings.global_rot_euler, ev);
		case OFFSET_ROT_LOCAL:
			return handle_vector(settings.local_rot_euler, ev);
		case HIP_MOVE:
			return handle_controller(ev);
		case HIP_MOVE_VECTOR:
			if (!associated_controller) return noneEvent;
			return handle_vector(associated_controller->analog_data, ev);
	}
	return noneEvent;
}

std::string RemoteTracker::get_description() {
	return "Tracker " + std::to_string(id);
}

const Basis& RemoteTracker::get_last_basis() {
	return last_basis;
}


void RemoteTracker::update_pose_if_needed(TrackedDevicePose_t* poses) {
	try {
		dataserver->tick();
	}
	catch (std::system_error& e) {
		DriverLog("*** TICK FAILED ***");
		DriverLog(e.what());
	}

	if (!dataserver->isDataAvailable()) {
		if (!dataserver->isConnectionAlive()) {
			send_invalid_pose();
		}
		return;
	}
	DriverPose_t pose = { 0 };
	pose.poseIsValid = true;
	pose.result = is_calibrating ? TrackingResult_Calibrating_InProgress : TrackingResult_Running_OK;
	pose.deviceIsConnected = true;


	Basis offset_basis;

	Vector3 offset_global = settings.offset_global;
	Vector3 offset_local_device = settings.offset_local_device;
	Vector3 offset_local_tracker = settings.offset_local_tracker;


	if ((settings.anchor_device_id >= 0) && (poses)) {
		TrackedDevicePose_t anchor = poses[settings.anchor_device_id];
		for (int i = 0; i < 3; i++) {
			pose.vecPosition[i] = anchor.mDeviceToAbsoluteTracking.m[i][3];
		}

		offset_basis.set(
			anchor.mDeviceToAbsoluteTracking.m[0][0],
			anchor.mDeviceToAbsoluteTracking.m[0][1],
			anchor.mDeviceToAbsoluteTracking.m[0][2],
			anchor.mDeviceToAbsoluteTracking.m[1][0],
			anchor.mDeviceToAbsoluteTracking.m[1][1],
			anchor.mDeviceToAbsoluteTracking.m[1][2],
			anchor.mDeviceToAbsoluteTracking.m[2][0],
			anchor.mDeviceToAbsoluteTracking.m[2][1],
			anchor.mDeviceToAbsoluteTracking.m[2][2]
		);
	}
	else {
		offset_basis.set_euler(Vector3());
		for (int i = 0; i < 3; i++) {
			pose.vecPosition[i] = 0.0;
			pose.vecVelocity[i] = 0.0;
		}
	}

	if (settings.x_override.enabled)
		pose.vecPosition[0] = settings.x_override.to;
	if (settings.y_override.enabled)
		pose.vecPosition[1] = settings.y_override.to;
	if (settings.z_override.enabled)
		pose.vecPosition[2] = settings.z_override.to;


	double* acceleration = dataserver->getAccel();
	pose.vecAcceleration[0] = acceleration[0];
	pose.vecAcceleration[1] = acceleration[1];
	pose.vecAcceleration[2] = acceleration[2];

	pose.qWorldFromDriverRotation = quaternion::init(0, 0, 0, 1);
	pose.qDriverFromHeadRotation = quaternion::init(0, 0, 0, 1);

	double* rotation = dataserver->getRotationQuaternion();

	Quat quat = Quat(rotation[0], rotation[1], rotation[2], rotation[3]);

	quat = Quat(Vector3(1, 0, 0), -Math_PI / 2.0) * quat;

	if (is_calibrating) {
		settings.global_rot_euler = Vector3(0, (get_yaw(quat)) - (get_yaw(offset_basis, Vector3(0, 0, -1))), 0);

		offset_global = (offset_basis.xform(Vector3(0, 0, -1)) * Vector3(1, 0, 1)).normalized() + Vector3(0, 0.2, 0);
		offset_local_device = Vector3(0, 0, 0);
		offset_local_tracker = Vector3(0, 0, 0);
	}

	quat = Quat(settings.global_rot_euler) * quat;


	if (is_down_calibrating) {
		float anchor_yaw = 0.0;
		if ((settings.anchor_device_id >= 0) && (poses)) {
			auto matrix = poses[settings.anchor_device_id].mDeviceToAbsoluteTracking;
			anchor_yaw = 
				get_yaw(from_hmdMatrix(matrix), Vector3(0, 0, -1));
		}
		
		auto rot = quat.inverse().get_euler_yxz();
		rot = (Quat(rot) * Quat(Vector3(0, 1, 0), -anchor_yaw)).get_euler_yxz();
		settings.local_rot_euler = rot;
	}

	quat = quat * Quat(settings.local_rot_euler);

	pose.qRotation = quaternion::from_Quat(quat);

	double* gyro = dataserver->getGyroscope();
	for (int i = 0; i < 3; i++) {
		pose.vecAngularVelocity[i] = 0;//gyro[i];
	}

	Basis final_tracker_basis = Basis(quat);
	last_basis = final_tracker_basis;

	for (int i = 0; i < 3; i++) {
		pose.vecPosition[i] += offset_global.get_axis(i);
		pose.vecPosition[i] += offset_basis.xform(offset_local_device).get_axis(i);
		pose.vecPosition[i] += final_tracker_basis.xform(offset_local_tracker).get_axis(i);
	}



	if ((!is_calibrating) && settings.should_predict_position) {
		Basis b = Basis(quat);
		Vector3 result = pos_predict.predict(*dataserver, b) * settings.position_prediction_strength;

		pose.vecPosition[0] += result.x;
		pose.vecPosition[1] += result.y;
		pose.vecPosition[2] += result.z;
	}

	VRServerDriverHost()->TrackedDevicePoseUpdated(m_unObjectId, pose, sizeof(pose));
}

void RemoteTracker::RunFrame(TrackedDevicePose_t* poses) {
	if (!activated) return;

	update_pose_if_needed(poses);
	if (associated_controller) {
		associated_controller->RunFrame(poses);
	}
}

void RemoteTracker::ProcessEvent(const vr::VREvent_t& vrEvent)
{
	switch (vrEvent.eventType)
	{
	case vr::VREvent_Input_HapticVibration:
	{
		if (vrEvent.data.hapticVibration.componentHandle == haptic)
		{
			dataserver->buzz(vrEvent.data.hapticVibration.fDurationSeconds, vrEvent.data.hapticVibration.fFrequency, vrEvent.data.hapticVibration.fAmplitude);
		}
	}
	break;
	}
}

const char* RemoteTracker::GetSerialNumber() const { return m_sSerialNumber.c_str(); }
const char* RemoteTracker::GetModelNumber() const { return m_sModelNumber.c_str(); }

const char* RemoteTracker::GetId() const { return GetSerialNumber(); }