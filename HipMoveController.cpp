#include "HipMoveController.h"

#include "RemoteTracker.h"

HipMoveController::HipMoveController(RemoteTracker* tgt) : tgt_tracker(tgt)
{
	m_unObjectId = vr::k_unTrackedDeviceIndexInvalid;
	m_ulPropertyContainer = vr::k_ulInvalidPropertyContainer;

	m_sSerialNumber = std::string(tgt->GetSerialNumber()) + "_HipMove";
	m_sModelNumber = std::string(tgt->GetModelNumber()) + "_HipMove";
}

HipMoveController::~HipMoveController(){}

EVRInitError HipMoveController::Activate(vr::TrackedDeviceIndex_t unObjectId)
{
	m_unObjectId = unObjectId;
	m_ulPropertyContainer = vr::VRProperties()->TrackedDeviceToPropertyContainer(m_unObjectId);

	vr::VRProperties()->SetUint64Property(m_ulPropertyContainer, vr::Prop_CurrentUniverseId_Uint64, 2);
	vr::VRProperties()->SetStringProperty(m_ulPropertyContainer, Prop_ModelNumber_String, "hip_locomotion");
	vr::VRProperties()->SetStringProperty(m_ulPropertyContainer, Prop_RenderModelName_String, "vr_controller_05_wireless_b");

	vr::VRProperties()->SetInt32Property(m_ulPropertyContainer, vr::Prop_ControllerRoleHint_Int32, vr::ETrackedControllerRole::TrackedControllerRole_Treadmill);

	vr::VRProperties()->SetStringProperty(m_ulPropertyContainer, Prop_InputProfilePath_String, "{owoTrack}/input/hipmove.json");

	vr::VRProperties()->SetBoolProperty(m_ulPropertyContainer, Prop_NeverTracked_Bool, true);

	vr::VRDriverInput()->CreateBooleanComponent(m_ulPropertyContainer, "/input/joystick/click", &joystick_click_component);
	vr::VRDriverInput()->CreateBooleanComponent(m_ulPropertyContainer, "/input/joystick/touch", &joystick_touch_component);
	vr::VRDriverInput()->CreateScalarComponent(m_ulPropertyContainer, "/input/joystick/x", &joystick_x_component, vr::EVRScalarType::VRScalarType_Absolute, vr::EVRScalarUnits::VRScalarUnits_NormalizedTwoSided);
	vr::VRDriverInput()->CreateScalarComponent(m_ulPropertyContainer, "/input/joystick/y", &joystick_y_component, vr::EVRScalarType::VRScalarType_Absolute, vr::EVRScalarUnits::VRScalarUnits_NormalizedTwoSided);


	return VRInitError_None;
}

void HipMoveController::Deactivate()
{
}

void HipMoveController::EnterStandby()
{
}

void* HipMoveController::GetComponent(const char* pchComponentNameAndVersion)
{
	return NULL;
}

void HipMoveController::PowerOff()
{
}

void HipMoveController::DebugRequest(const char* pchRequest, char* pchResponseBuffer, uint32_t unResponseBufferSize)
{
}

DriverPose_t HipMoveController::GetPose() {
	DriverPose_t pose = { 0 };
	pose.poseIsValid = false;
	pose.result = TrackingResult_Calibrating_OutOfRange;
	pose.deviceIsConnected = false;

	pose.vecPosition[0] = 9999999;
	pose.vecPosition[1] = 9999999;
	pose.vecPosition[2] = 9999999;


	return pose;
}

void HipMoveController::RunFrame(TrackedDevicePose_t* poses)
{
	if (!enabled) {
		SetDirection(analog_data.x, analog_data.y);
		return;
	}


	float hmdYaw = get_yaw(from_hmdMatrix(poses[0].mDeviceToAbsoluteTracking), Vector3(0, 0, -1));
	float trackerYaw = get_yaw(tgt_tracker->get_last_basis(), Vector3(0, 0, -1));

	float diff = trackerYaw - hmdYaw - 3.1415926535898;

	float magnitude = sqrt(analog_data.x * analog_data.x + analog_data.y * analog_data.y);
	float angle = atan2(analog_data.y, analog_data.x);

	float new_x = cos(angle - diff) * magnitude;
	float new_y = sin(angle - diff) * magnitude;


	DriverLog("HipMove enabled, (hmd %.2f, tracker %.2f).... initial (%.2f, %.2f), diff (%.2f), new (%.2f, %.2f)", hmdYaw, trackerYaw, analog_data.x, analog_data.y, diff, new_x, new_y);

	SetDirection(new_x, new_y);
}

void HipMoveController::ProcessEvent(const vr::VREvent_t& vrEvent)
{
}

const char* HipMoveController::GetSerialNumber() const { return m_sSerialNumber.c_str(); }
const char* HipMoveController::GetModelNumber() const {	return m_sModelNumber.c_str(); }
const char* HipMoveController::GetId() const { return GetSerialNumber(); }


void HipMoveController::SetDirection(float x, float y)
{
	vr::VRDriverInput()->UpdateScalarComponent(joystick_x_component, x, -1);
	vr::VRDriverInput()->UpdateScalarComponent(joystick_y_component, y, -1);

	vr::VRDriverInput()->UpdateBooleanComponent(joystick_touch_component, sqrt(x * x + y * y) > 0.05f, -1);
}