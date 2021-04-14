#pragma once

#include <openvr_driver.h>
#include "AbstractDevice.h"
#include "vector3.h"

class RemoteTracker;

class HipMoveController : public AbstractDevice {
	private:
		RemoteTracker* tgt_tracker;

		vr::VRInputComponentHandle_t joystick_click_component = 0;
		vr::VRInputComponentHandle_t joystick_touch_component = 0;
		vr::VRInputComponentHandle_t joystick_x_component = 0;
		vr::VRInputComponentHandle_t joystick_y_component = 0;

		void SetDirection(float x, float y);


	public:
		HipMoveController(RemoteTracker* tgt);
		~HipMoveController();


		EVRInitError Activate(vr::TrackedDeviceIndex_t unObjectId) override;

		void Deactivate() override;
		void EnterStandby() override;

		void* GetComponent(const char* pchComponentNameAndVersion) override;

		void PowerOff() override;
		void DebugRequest(const char* pchRequest, char* pchResponseBuffer, uint32_t unResponseBufferSize) override;

		DriverPose_t GetPose() override;

		void RunFrame(TrackedDevicePose_t* poses) override;

		void ProcessEvent(const vr::VREvent_t& vrEvent) override;

		const char* GetSerialNumber() const override;
		const char* GetModelNumber() const override;
		const char* GetId() const override;

		bool enabled = false;
		Vector3 analog_data;

};