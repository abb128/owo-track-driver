#pragma once

#include <openvr_driver.h>
#include "driverlog.h"
#include "util.h"


class AbstractDevice : public vr::ITrackedDeviceServerDriver {
	protected:
		vr::TrackedDeviceIndex_t m_unObjectId;
		vr::PropertyContainerHandle_t m_ulPropertyContainer;

		std::string m_sSerialNumber;
		std::string m_sModelNumber;

	public:
		virtual EVRInitError Activate(vr::TrackedDeviceIndex_t unObjectId) = 0;
		virtual void Deactivate() = 0;
		virtual void EnterStandby() = 0;
		virtual void* GetComponent(const char* pchComponentNameAndVersion) = 0;
		virtual void PowerOff() = 0;
		virtual void DebugRequest(const char* pchRequest, char* pchResponseBuffer, uint32_t unResponseBufferSize) = 0;
		virtual DriverPose_t GetPose() = 0;
		virtual void RunFrame(TrackedDevicePose_t* poses) = 0;
		virtual void ProcessEvent(const vr::VREvent_t& vrEvent) = 0;

		virtual const char* GetModelNumber() const = 0;
		virtual const char* GetSerialNumber() const = 0;
		virtual const char* GetId() const = 0;
};
