#pragma once

#include "vector3.h"
#include "DeviceQuatServer.h"
#include "basis.h"

class PositionPredictor {
private:
	Vector3 gyro = Vector3();
	Vector3 position = Vector3();
	Vector3 velocity = Vector3();
	Vector3 acceleration = Vector3();

public:
	Vector3 predict(DeviceQuatServer& serv, Basis& basis);
};