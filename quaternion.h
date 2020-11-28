#pragma once

#include <openvr_driver.h>
#include <cmath>

#include "quat.h"

namespace quaternion
{
    inline vr::HmdQuaternion_t fromHmdMatrix34(vr::HmdMatrix34_t matrix)
    {
        vr::HmdQuaternion_t q;

        q.w = sqrt(fmax(0,
            static_cast<double>(1 + matrix.m[0][0] + matrix.m[1][1]
                + matrix.m[2][2])))
            / 2;
        q.x = sqrt(fmax(0,
            static_cast<double>(1 + matrix.m[0][0] - matrix.m[1][1]
                - matrix.m[2][2])))
            / 2;
        q.y = sqrt(fmax(0,
            static_cast<double>(1 - matrix.m[0][0] + matrix.m[1][1]
                - matrix.m[2][2])))
            / 2;
        q.z = sqrt(fmax(0,
            static_cast<double>(1 - matrix.m[0][0] - matrix.m[1][1]
                + matrix.m[2][2])))
            / 2;
        q.x = copysign(q.x,
            static_cast<double>(matrix.m[2][1] - matrix.m[1][2]));
        q.y = copysign(q.y,
            static_cast<double>(matrix.m[0][2] - matrix.m[2][0]));
        q.z = copysign(q.z,
            static_cast<double>(matrix.m[1][0] - matrix.m[0][1]));
        return q;
    }

    inline vr::HmdQuaternion_t multiply(const vr::HmdQuaternion_t& lhs,
        const vr::HmdQuaternion_t& q)
    {

        double nw = lhs.w * q.w - lhs.x * q.x - lhs.y * q.y - lhs.z * q.z;
        double nx = lhs.w * q.x + lhs.x * q.w + lhs.y * q.z - lhs.z * q.y;
        double ny = lhs.w * q.y + lhs.y * q.w + lhs.z * q.x - lhs.x * q.z;

        return { nw, nx, ny, lhs.w * q.z + lhs.z * q.w + lhs.x * q.y - lhs.y * q.x };
    }

    inline vr::HmdQuaternion_t conjugate(const vr::HmdQuaternion_t& quat)
    {
        return {
            quat.w,
            -quat.x,
            -quat.y,
            -quat.z,
        };
    }

    inline double getYaw(const vr::HmdQuaternion_t& quat)
    {
        double yawResult
            = atan2(2.0 * (quat.y * quat.w + quat.x * quat.z),
                (2.0 * (quat.w * quat.w + quat.x * quat.x)) - 1.0);
        return yawResult;
    }

    inline double getPitch(const vr::HmdQuaternion_t& quat)
    {
        // positive forward
        // negative behind

        double pitchResult
            = atan2(2.0 * (quat.x * quat.w + quat.y * quat.z),
                1.0 - 2.0 * (quat.x * quat.x + quat.y * quat.y));
        //    double pitchResult
        //= atan2( 2.0 * ( quat.x * quat.w + quat.y * quat.z ),
        //      2.0 * ( quat.w * quat.w + quat.z * quat.z ) - 1.0 );
        return pitchResult;
    }

    inline double getRoll(const vr::HmdQuaternion_t& quat)
    {
        double rollResult;
        double sinp = 2 * (quat.w * quat.z - quat.y * quat.x);
        if (std::abs(sinp) >= 1)

        {
            rollResult = std::copysign(3.14159265358979323846 / 2, sinp);
        }
        else
        {
            rollResult = std::asin(sinp);
        }
        return rollResult;
    }

    inline vr::HmdQuaternion_t fromAxisAngle(double x, double y, double z, double a) {
        double s = sin(a / 2.0);
        return { cos(a / 2.0), x * s, y * s, z * s };
    }

    inline vr::HmdQuaternion_t init(double x, double y, double z, double w) {
        return { w, x, y, z };
    }


    inline vr::HmdQuaternion_t from_Quat(Quat q) {
        return init(q.x, q.y, q.z, q.w);
    }
    inline Quat to_Quat(vr::HmdQuaternion_t q) {
        return Quat(q.x, q.y, q.z, q.w);
    }
} // namespace quaternion
