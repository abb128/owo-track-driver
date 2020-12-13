#pragma once

// abstract class so other implementations can be made
// (bluetooth, etc)

class DeviceQuatServer {
public:
	virtual void startListening() = 0; // set up server
	virtual void tick() = 0; // tick

	virtual bool isDataAvailable() = 0; // true if new data is available
	virtual double* getRotationQuaternion() = 0; // rotation quat {x, y, z, w}
	virtual double* getGyroscope() = 0; // gyro rad/s {x, y, z}
	virtual double* getAccel() = 0; // accelerometer m/s^2 {x, y, z}

	virtual bool isConnectionAlive() = 0; // checks if connection is still alive

	virtual void buzz(float duration_s, float frequency, float amplitude) = 0; // vibrates

	virtual int get_port() = 0; // returns port or other unique id
};