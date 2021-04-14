#pragma once


#include <openvr_driver.h>
#include "driverlog.h"

#include "util.h"

#include "DeviceQuatServer.h"
#include "RemoteTrackerSettings.h"

#include "PositionPredictor.h"

#include "owoIPC.h"

#include "AbstractDevice.h"

#include "HipMoveController.h"

class RemoteTracker : public AbstractDevice {
	private:
		vr::VRInputComponentHandle_t haptic;

		RemoteTrackerSettings settings;
		DeviceQuatServer* dataserver;
		PositionPredictor pos_predict;

		bool is_calibrating = false;
		bool is_down_calibrating = false;

		Basis last_basis;

		HipMoveController* associated_controller = 0;

		owoEvent handle_controller(owoEvent ev);

		template<typename T>
		owoEvent give_value(T local_val, owoEvent ev);

		template <typename T>
		owoEvent set_setting_or_give_value(T& local_val, owoEvent ev);

		owoEvent handle_vector(Vector3& local_val, owoEvent ev);

		void update_pose_if_needed(TrackedDevicePose_t* poses);

	public:
		unsigned int id = 0;
		unsigned int port_no = 0;

		RemoteTracker(DeviceQuatServer* server, const int& id, RemoteTrackerSettings settings_v);
		~RemoteTracker();

		EVRInitError Activate(vr::TrackedDeviceIndex_t unObjectId) override;

		void Deactivate() override;
		void EnterStandby() override;

		void* GetComponent(const char* pchComponentNameAndVersion) override;

		void PowerOff() override;

		/** debug request from a client */
		void DebugRequest(const char* pchRequest, char* pchResponseBuffer, uint32_t unResponseBufferSize) override;

		DriverPose_t GetPose() override;

		void RunFrame(TrackedDevicePose_t *poses) override;

		void ProcessEvent(const vr::VREvent_t& vrEvent) override;

		const char* GetSerialNumber() const override;
		const char* GetModelNumber() const override;
		const char* GetId() const override;

		void send_invalid_pose();
		owoEvent process_request(owoEvent ev);
		std::string get_description();

		const Basis& get_last_basis();
};
