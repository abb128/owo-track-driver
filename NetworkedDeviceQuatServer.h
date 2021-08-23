#pragma once

#include "DeviceQuatServer.h"

#define MSG_HEARTBEAT 0
#define MSG_ROTATION 1
#define MSG_GYRO 2
#define MSG_HANDSHAKE 3
#define MSG_ACCELEROMETER 4

typedef unsigned int message_header_type_t;
typedef unsigned long long message_id_t;
typedef float sensor_data_t;

// message type + packet id
#define MSG_HEADER_SIZE (sizeof(message_header_type_t) + sizeof(message_id_t))

// SlimeVR extensions add some more, just stick with 256 for now
#define MAX_MSG_SIZE 256

/*
first 4 bytes - message type
( 0 = heartbeat
  1 = rotation
  2 = gyro
  3 = handshake
  4 = accelerometer)

next 8 bytes - packet id (+1 every time)

rotation:
(floats)
next 4 bytes - rotation quat x
next 4 bytes - rotation quat y
next 4 bytes - rotation quat z
next 4 bytes - rotation quat w

gyro:
(floats)
next 4 bytes - gyro rate x
next 4 bytes - gyro rate y
next 4 bytes - gyro rate z


64 byte packets
*/

template<typename T>
T convert_chars(unsigned char* src) {
	union {
		unsigned char c[sizeof(T)];
		T v;
	} un;
	for (int i = 0; i < sizeof(T); i++) {
		un.c[i] = src[sizeof(T) - i - 1];
	}
	return un.v;
}


class NetworkedDeviceQuatServer : public DeviceQuatServer {
private:
	message_id_t current_packet_id = 0;

	double* quat_buffer; // size 4
	double* gyro_buffer; // size 3
	double* accel_buffer; // size 3

	bool receive_packet_id(message_id_t new_id);

	bool isNewDataAvailable = false;

	void handle_doubles_packet(unsigned char* packet, double* into, int num_doubles);

protected:
	void handle_gyro_packet(unsigned char* packet);
	void handle_accel_packet(unsigned char* packet);
	void handle_rotation_packet(unsigned char* packet);


	char* buff_hello;
	int buff_hello_len;

public:
	NetworkedDeviceQuatServer();

	bool isDataAvailable();
	double* getRotationQuaternion();
	double* getGyroscope();
	double* getAccel();
};

#define HEARTBEAT_THRESHOLD 1000
#define DEAD_THRESHOLD HEARTBEAT_THRESHOLD*10
