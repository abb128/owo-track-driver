#pragma once

#include <openvr_driver.h>
#include "quat.h"
#include "basis.h"

using namespace vr;

inline void HmdMatrix_SetIdentity(HmdMatrix34_t* pMatrix){
	pMatrix->m[0][0] = 1.f;
	pMatrix->m[0][1] = 0.f;
	pMatrix->m[0][2] = 0.f;
	pMatrix->m[0][3] = 0.f;
	pMatrix->m[1][0] = 0.f;
	pMatrix->m[1][1] = 1.f;
	pMatrix->m[1][2] = 0.f;
	pMatrix->m[1][3] = 0.f;
	pMatrix->m[2][0] = 0.f;
	pMatrix->m[2][1] = 0.f;
	pMatrix->m[2][2] = 1.f;
	pMatrix->m[2][3] = 0.f;
}

inline double get_yaw(Basis basis, Vector3 front_v) {
	// xform to get front vector (up points front)
	Vector3 front_relative = basis.xform(front_v);


	// flatten to XZ for yaw
	front_relative = (front_relative * Vector3(1, 0, 1)).normalized();

	// get angle
	double angle = front_relative.angle_to(Vector3(0, 0, 1));

	// convert to offset (idk why front_relative.x works)
	return -angle * Math::sign(front_relative.x);
}

inline double get_yaw(Quat quat) {
	return get_yaw(Basis(quat), Vector3(0, 1, 0));
}

inline Basis from_hmdMatrix(const HmdMatrix34_t &matrix) {
	return Basis(
		matrix.m[0][0], matrix.m[0][1], matrix.m[0][2],
		matrix.m[1][0], matrix.m[1][1], matrix.m[1][2],
		matrix.m[2][0], matrix.m[2][1], matrix.m[2][2]
	);
}
