#pragma once


#include <openvr_driver.h>
#include "driverlog.h"

#include "util.h"

#include "DeviceQuatServer.h"
#include "RemoteTrackerSettings.h"

#include "PositionPredictor.h"

#include "owoIPC.h"

using namespace vr;

class RemoteTracker : public vr::ITrackedDeviceServerDriver {
private:
	vr::TrackedDeviceIndex_t m_unObjectId;
	vr::PropertyContainerHandle_t m_ulPropertyContainer;

	vr::VRInputComponentHandle_t m_compHaptic;

	std::string m_sSerialNumber;
	std::string m_sModelNumber;

	DeviceQuatServer* dataserver;

	vr::HmdQuaternion_t worldFromDriverRot;

	RemoteTrackerSettings settings;

	PositionPredictor pos_predict;

	bool is_calibrating = false;


	template<typename T>
	owoEvent give_value(T local_val, owoEvent ev);

	template <typename T>
	owoEvent set_setting_or_give_value(T& local_val, owoEvent ev);

	owoEvent handle_vector(Vector3& local_val, owoEvent ev);

public:
	unsigned int id = 0;
	unsigned int port_no = 0;

	RemoteTracker(DeviceQuatServer* server, const int& id, RemoteTrackerSettings settings_v);

	virtual ~RemoteTracker();


	virtual EVRInitError Activate(vr::TrackedDeviceIndex_t unObjectId);

	virtual void Deactivate();

	virtual void EnterStandby();

	void* GetComponent(const char* pchComponentNameAndVersion);

	virtual void PowerOff();

	/** debug request from a client */
	virtual void DebugRequest(const char* pchRequest, char* pchResponseBuffer, uint32_t unResponseBufferSize);

	virtual DriverPose_t GetPose();

	void RunFrame(TrackedDevicePose_t *poses);

	void ProcessEvent(const vr::VREvent_t& vrEvent);
	std::string GetSerialNumber() const;

	void send_invalid_pose();

	owoEvent process_request(owoEvent ev);
};
