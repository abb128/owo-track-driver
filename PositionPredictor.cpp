#include "PositionPredictor.h"

#define MINIMI_RES 0.12

inline double minimize_val(double v) {
	return (abs(v) < MINIMI_RES) ? 0 : v - MINIMI_RES;
}

inline Vector3 minimize_vector(Vector3 v) {
	return Vector3(
		minimize_val(v.x),
		minimize_val(v.y),
		minimize_val(v.z)
	);
}



Vector3 PositionPredictor::predict(DeviceQuatServer& serv, Basis& basis){
	double *gyro_a = serv.getGyroscope();
	double *accel_a = serv.getAccel();
	gyro = gyro.lerp(Vector3(gyro_a[0], gyro_a[1], gyro_a[2]), 0.1);
	acceleration = acceleration.lerp(Vector3(accel_a[0], accel_a[1], accel_a[2]), 0.4);

	Vector3 accel_local = acceleration / (1.0 + gyro.length_squared() * 4.0);
	accel_local = minimize_vector(accel_local);

	velocity += basis.xform(accel_local);
	velocity = minimize_vector(velocity);
	velocity /= 1.12;

	position = position.lerp((position + velocity * 3.0) / 1.6, 0.05);

	return position / 100.0;
}
