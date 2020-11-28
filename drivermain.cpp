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

		UDPDeviceQuatServer* server = new UDPDeviceQuatServer(port);
		RemoteTracker *tracker = new RemoteTracker(server, id, defaults);
		vr::VRServerDriverHost()->TrackedDeviceAdded(tracker->GetSerialNumber().c_str(), vr::TrackedDeviceClass_GenericTracker, tracker);
		
		trackers.insert({ id, tracker });
	}
	int primary_tracker_added = 0;

	std::map<int, RemoteTracker*> trackers;
	TrackedDevicePose_t* poses;
};

RemoteTrackerServerDriver g_serverDriverNull;


EVRInitError RemoteTrackerServerDriver::Init( vr::IVRDriverContext *pDriverContext )
{
	VR_INIT_SERVER_DRIVER_CONTEXT( pDriverContext );
	InitDriverLog( vr::VRDriverLog() );
	poses = (TrackedDevicePose_t *)malloc(sizeof(TrackedDevicePose_t) * k_unMaxTrackedDeviceCount);


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


	// we absollutely do NOT want to be added as the first tracker, as it may break stuff while
	// controllers are not present
	if (primary_tracker_added < 400) {
		primary_tracker_added += 1;
	}
	else if (primary_tracker_added < 5000) {
		primary_tracker_added = 100000;
		add_tracker(0, 6969);
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


