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

RemoteTracker::RemoteTracker(DeviceQuatServer *server, const int& id, RemoteTrackerSettings settings_v) {
	m_unObjectId = vr::k_unTrackedDeviceIndexInvalid;
	m_ulPropertyContainer = vr::k_ulInvalidPropertyContainer;

	m_sSerialNumber = "VIRT_TRACK0" + std::to_string(id);

	m_sModelNumber = "VirtualTracker" + std::to_string(id);

	dataserver = server;
	settings = settings_v;

	worldFromDriverRot = quaternion::init(0, 0, 0, 1);
}

RemoteTracker::~RemoteTracker()
{
	delete dataserver;
}

EVRInitError RemoteTracker::Activate(vr::TrackedDeviceIndex_t unObjectId)
{
	m_unObjectId = unObjectId;
	m_ulPropertyContainer = vr::VRProperties()->TrackedDeviceToPropertyContainer(m_unObjectId);

	vr::VRProperties()->SetStringProperty(m_ulPropertyContainer, Prop_ModelNumber_String, "Vive Tracker Pro MV");
	vr::VRProperties()->SetStringProperty(m_ulPropertyContainer, Prop_RenderModelName_String, "{htc}vr_tracker_vive_1_0");

	// return a constant that's not 0 (invalid) or 1 (reserved for Oculus)
	vr::VRProperties()->SetUint64Property(m_ulPropertyContainer, Prop_CurrentUniverseId_Uint64, 2);

	vr::VRProperties()->SetBoolProperty(m_ulPropertyContainer, Prop_IsOnDesktop_Bool, false);

	vr::VRProperties()->SetBoolProperty(m_ulPropertyContainer, Prop_NeverTracked_Bool, false);

	vr::VRProperties()->SetInt32Property(m_ulPropertyContainer, Prop_ControllerRoleHint_Int32, TrackedControllerRole_OptOut);

	vr::VRProperties()->SetStringProperty(m_ulPropertyContainer, Prop_InputProfilePath_String, "{sample}/input/mycontroller_profile.json");

	// create our haptic component
	vr::VRDriverInput()->CreateHapticComponent(m_ulPropertyContainer, "/output/haptic", &m_compHaptic);

	try {
		dataserver->startListening();
	}
	catch (std::system_error& e) {
		DriverLog("*** LISTEN FAILED ***");
		DriverLog(e.what());
		return VRInitError_Driver_Failed;
	}

	return VRInitError_None;
}

void RemoteTracker::Deactivate()
{
	m_unObjectId = vr::k_unTrackedDeviceIndexInvalid;
}

void RemoteTracker::EnterStandby()
{
}

void* RemoteTracker::GetComponent(const char* pchComponentNameAndVersion)
{
	// override this to add a component to a driver
	return NULL;
}

void RemoteTracker::PowerOff()
{
}

/** debug request from a client */

enum MESSAGE_TYPES {
	SET_CALIBRATION_MODE = 1,
	GET_CALIBRATION_MODE = 2,
	
	SET_POSITION_PREDICTION = 3,
	GET_POSITION_PREDICTION = 4,

	GET_DRIVER_VERSION = 5,

	SET_CALIBRATION_YAW = 6,
	GET_CALIBRATION_YAW = 7,

	GET_CONNECTION_ALIVE = 8
};

void RemoteTracker::DebugRequest(const char* pchRequest, char* pchResponseBuffer, uint32_t unResponseBufferSize)
{
	unsigned int msg_type = pchRequest[0];
	switch (msg_type) {

		case SET_CALIBRATION_MODE:
			is_calibrating = pchRequest[1] == 2;
			return;
		case GET_CALIBRATION_MODE:
			pchResponseBuffer[0] = (is_calibrating)+1;
			return;

		case SET_POSITION_PREDICTION:
			settings.should_predict_position = pchRequest[1] == 2;
			return;
		case GET_POSITION_PREDICTION:
			pchResponseBuffer[0] = (settings.should_predict_position)+1;
			return;

		case GET_DRIVER_VERSION:
			pchResponseBuffer[0] = 1;
			return;

		case SET_CALIBRATION_YAW: {
			std::string data(pchRequest + 1);
			DriverLog("recv string %s", data.c_str());
			double new_yaw = std::stod(data);
			DriverLog("Set to %s", data.c_str());
			settings.yaw_offset = new_yaw;
			return;
		}
		case GET_CALIBRATION_YAW: {
			std::string data = std::to_string(settings.yaw_offset);
			const char* data_c = data.c_str();
			int i;
			for (i = 0; i < min(unResponseBufferSize, data.length()); i++) {
				pchResponseBuffer[i] = data_c[i];
			}
			pchResponseBuffer[i] = 0;
			return;
		}

		case GET_CONNECTION_ALIVE: {
			pchResponseBuffer[0] = dataserver->isConnectionAlive() ? 2 : 1;
			return;
		}


		default:
			DriverLog("Unknown message type %d", msg_type);
			pchResponseBuffer[0] = 0;
	}
}

DriverPose_t RemoteTracker::GetPose()
{
	DriverPose_t pose = { 0 };
	pose.poseIsValid = true;
	pose.result = TrackingResult_Running_OutOfRange;
	pose.deviceIsConnected = true;

	pose.vecPosition[0] = 0.0;
	pose.vecPosition[1] = 1.5;
	pose.vecPosition[2] = 0.0;

	pose.vecVelocity[0] = 0.0;
	pose.vecVelocity[1] = 0.0;
	pose.vecVelocity[2] = 0.0;

	pose.qWorldFromDriverRotation = quaternion::init(0, 0, 0, 1);
	pose.qDriverFromHeadRotation = quaternion::init(0, 0, 0, 1);

	return pose;
}

void RemoteTracker::RunFrame(TrackedDevicePose_t* poses)
{
	try {
		dataserver->tick();
	}
	catch (std::system_error& e) {
		DriverLog("*** TICK FAILED ***");
		DriverLog(e.what());
	}

	if (!dataserver->isDataAvailable()) {
		return;
	}
	DriverPose_t pose = { 0 };
	pose.poseIsValid = true;
	pose.result = TrackingResult_Running_OK;
	pose.deviceIsConnected = true;


	if ((settings.anchor_device_id >= 0) && (poses)) {
		TrackedDevicePose_t anchor = poses[settings.anchor_device_id];
		for (int i = 0; i < 3; i++) {
			pose.vecPosition[i] = anchor.mDeviceToAbsoluteTracking.m[i][3];
		}

		/*
		auto props = vr::VRProperties()->TrackedDeviceToPropertyContainer(settings.anchor_device_id);

		std::string modelNumber = VRProperties()->GetStringProperty(props, Prop_ModelNumber_String);
		std::string renderModel = VRProperties()->GetStringProperty(props, Prop_RenderModelName_String);

		DriverLog("Device %d = %s %s (%.2f %.2f %.2f)\n", settings.anchor_device_id, modelNumber.c_str(), renderModel.c_str(),
			anchor.mDeviceToAbsoluteTracking.m[0][3],
			anchor.mDeviceToAbsoluteTracking.m[1][3],
			anchor.mDeviceToAbsoluteTracking.m[2][3]
		);
		*/
	}
	else {
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


	for (int i = 0; i < 3; i++) {
		pose.vecPosition[i] += settings.offset_position[i];
	}

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
		Basis basis(quat);
		Vector3 front(0, 0, 1);

		// xform to get front vector (up points front)
		Vector3 front_relative = basis.xform(Vector3(0, 1, 0));

		//DriverLog("Front: %.2f   %.2f   %.2f ", front_relative.x, front_relative.y, front_relative.z);


		// flatten to XZ for yaw
		front_relative = (front_relative * Vector3(1, 0, 1)).normalized();

		// get angle
		double angle = front_relative.angle_to(front);

		//DriverLog("Angle: %.3f ", angle);

		// convert to offset (idk why front_relative.x works)
		settings.yaw_offset = -angle * Math::sign(front_relative.x);

		for (int i = 0; i < 3; i++) {
			pose.vecPosition[i] = 0.0;
		}
		pose.vecPosition[1] = 1.5;
	}

	//DriverLog("Yaw_Offset = %.2f ", settings.yaw_offset);

	quat = Quat(Vector3(0, 1, 0), settings.yaw_offset) * quat;

	pose.qRotation = quaternion::from_Quat(quat);

	double* gyro = dataserver->getGyroscope();
	for (int i = 0; i < 3; i++) {
		pose.vecAngularVelocity[i] = 0;//gyro[i];
	}


	if ((!is_calibrating) && settings.should_predict_position) {
		Vector3 result = pos_predict.predict(*dataserver, Basis(quat)) * settings.position_prediction_strength;

		pose.vecPosition[0] += result.x;
		pose.vecPosition[1] += result.y;
		pose.vecPosition[2] += result.z;
	}

	VRServerDriverHost()->TrackedDevicePoseUpdated(m_unObjectId, pose, sizeof(pose));



}

void RemoteTracker::ProcessEvent(const vr::VREvent_t& vrEvent)
{
	switch (vrEvent.eventType)
	{
	case vr::VREvent_Input_HapticVibration:
	{
		if (vrEvent.data.hapticVibration.componentHandle == m_compHaptic)
		{
			// This is where you would send a signal to your hardware to trigger actual haptic feedback
			DriverLog("BUZZ!\n");
			dataserver->buzz(vrEvent.data.hapticVibration.fDurationSeconds, vrEvent.data.hapticVibration.fFrequency, vrEvent.data.hapticVibration.fAmplitude);
		}
	}
	break;
	}
}

std::string RemoteTracker::GetSerialNumber() const { return m_sSerialNumber; }
