#pragma once
#include <openvr_driver.h>
using namespace vr;

// override an axis
struct AxisOverride {
	bool enabled = false;
	double to = 0.0;
};

struct RemoteTrackerSettings {
	// device to anchor to
	// if -1 then it should be treated as having no anchor
	// with all offsets done from (0, 0, 0)
	unsigned int anchor_device_id;

	// meters to offset from the anchor device
	double offset_position[3];

	// if true, uses global space to perform offsets
	// if false, the offset is done relative to the rotation
	// of anchor device
	bool use_global_offset_space;

	// yaw offset (true north vs vr north)
	double yaw_offset;

	// override axises
	AxisOverride x_override = { false, 0.0 };
	AxisOverride y_override = { false, 0.0 };
	AxisOverride z_override = { false, 0.0 };

	// predict position?
	bool should_predict_position = true;
	double position_prediction_strength = 1.0;
};
