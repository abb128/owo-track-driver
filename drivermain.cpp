#include "UDPDeviceQuatServer.h"
#include "DeviceQuatServer.h"

#include <sstream>

#include <openvr_driver.h>
#include "driverlog.h"

#include "util.h"
#include "RemoteTracker.h"

#include <map>

#if defined( _WINDOWS )
#include <windows.h>
#endif



using namespace vr;

#if defined(_WIN32)
#define HMD_DLL_EXPORT extern "C" __declspec( dllexport )
#define HMD_DLL_IMPORT extern "C" __declspec( dllimport )
#endif


#include "RemoteTrackerSettings.h"
#include <thread>

class RemoteTrackerServerDriver: public IServerTrackedDeviceProvider
{
public:
	virtual EVRInitError Init( vr::IVRDriverContext *pDriverContext ) ;
	virtual void Cleanup() ;
	virtual const char * const *GetInterfaceVersions() { return vr::k_InterfaceVersions; }
	virtual void RunFrame() ;
	virtual bool ShouldBlockStandbyMode()  { return false; }
	virtual void EnterStandby()  {}
	virtual void LeaveStandby()  {}

private:
	void add_tracker(const int& id, const int& port) {
		RemoteTrackerSettings defaults;

		defaults.anchor_device_id = 0;
		defaults.offset_position[0] = 0.0;
		defaults.offset_position[1] = -0.70;
		defaults.offset_position[2] = 0.0;

		defaults.use_global_offset_space = true;
		defaults.yaw_offset = 0.0;

		defaults.should_predict_position = false;

		UDPDeviceQuatServer* server = new UDPDeviceQuatServer(port);
		RemoteTracker *tracker = new RemoteTracker(server, id, defaults);
		vr::VRServerDriverHost()->TrackedDeviceAdded(tracker->GetSerialNumber().c_str(), vr::TrackedDeviceClass_GenericTracker, tracker);
		
		trackers.insert({ id, tracker });
	}

	std::map<int, RemoteTracker*> trackers;
	TrackedDevicePose_t* poses;
};

RemoteTrackerServerDriver g_serverDriverNull;


EVRInitError RemoteTrackerServerDriver::Init( vr::IVRDriverContext *pDriverContext )
{
	VR_INIT_SERVER_DRIVER_CONTEXT( pDriverContext );
	InitDriverLog( vr::VRDriverLog() );
	poses = (TrackedDevicePose_t *)malloc(sizeof(TrackedDevicePose_t) * k_unMaxTrackedDeviceCount);


	std::thread* tracker_init = new std::thread([&] {

		// wait until controllers are found
		// so as to avoid stealing input
		while (true) {
			std::this_thread::sleep_for(std::chrono::milliseconds(1000));

			int hands_count = 0;
			for (int i = 0; i < k_unMaxTrackedDeviceCount; i++) {
				auto propertyContainer = vr::VRProperties()->TrackedDeviceToPropertyContainer(i);

				ETrackedPropertyError err;

				ETrackedDeviceClass deviceClass = (ETrackedDeviceClass)
						vr::VRProperties()->GetInt32Property(propertyContainer, Prop_DeviceClass_Int32, &err);
				if (err) continue;

				if (deviceClass != TrackedDeviceClass_Controller) continue;

				ETrackedControllerRole role = (ETrackedControllerRole)
						vr::VRProperties()->GetInt32Property(propertyContainer, Prop_ControllerRoleHint_Int32, &err);
				if (err) continue;

				if ((role == TrackedControllerRole_LeftHand) || (role == TrackedControllerRole_RightHand)) {
					hands_count++;
				}
			}

			if (hands_count >= 2) {
				// wait for a moment longer because i dont trust like that
				std::this_thread::sleep_for(std::chrono::milliseconds(3000));
				break;
			}
		}
		add_tracker(0, 6969);

		return;
	});

	return VRInitError_None;
}

void RemoteTrackerServerDriver::Cleanup() 
{
	CleanupDriverLog();
	for (auto v : trackers) {
		delete ((RemoteTracker*)(v.second));
	}
}


void RemoteTrackerServerDriver::RunFrame()
{
	VRServerDriverHost()->GetRawTrackedDevicePoses(0, poses, k_unMaxTrackedDeviceCount);

	for (auto v : trackers) {
		((RemoteTracker*)v.second)->RunFrame(poses);
	}

	vr::VREvent_t vrEvent;
	while ( vr::VRServerDriverHost()->PollNextEvent( &vrEvent, sizeof( vrEvent ) ) )
	{
		for (auto v : trackers) {
			((RemoteTracker*)v.second)->ProcessEvent(vrEvent);
		}
	}
}




HMD_DLL_EXPORT void *HmdDriverFactory( const char *pInterfaceName, int *pReturnCode )
{
	if( 0 == strcmp( IServerTrackedDeviceProvider_Version, pInterfaceName ) )
	{
		return &g_serverDriverNull;
	}

	if( pReturnCode )
		*pReturnCode = VRInitError_Init_InterfaceNotFound;

	return NULL;
}


