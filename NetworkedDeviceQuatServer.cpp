#include "NetworkedDeviceQuatServer.h"
#include <stdlib.h>

bool NetworkedDeviceQuatServer::receive_packet_id(message_id_t new_id) {
	if ((new_id > current_packet_id) || (new_id < 5)) {
		current_packet_id = new_id;
		return true;
	}
	return false;
}

void NetworkedDeviceQuatServer::handle_doubles_packet(unsigned char* packet, double* into, int num_doubles) {
	packet += sizeof(message_header_type_t);

	message_id_t id = convert_chars<message_id_t>(packet);
	packet += sizeof(message_id_t);
	if (!receive_packet_id(id)) return;

	for (int i = 0; i < num_doubles; i++) {
		sensor_data_t data = convert_chars<sensor_data_t>(packet);
		packet += sizeof(sensor_data_t);
		into[i] = (double)data;
	}

	isNewDataAvailable = true;
}


void NetworkedDeviceQuatServer::handle_gyro_packet(unsigned char* packet){
	handle_doubles_packet(packet, gyro_buffer, 3);
}
void NetworkedDeviceQuatServer::handle_rotation_packet(unsigned char* packet){
	handle_doubles_packet(packet, quat_buffer, 4);
}
void NetworkedDeviceQuatServer::handle_accel_packet(unsigned char* packet){
	handle_doubles_packet(packet, accel_buffer, 3);
}


bool NetworkedDeviceQuatServer::isDataAvailable() {
	bool was_available = isNewDataAvailable;
	isNewDataAvailable = false;
	return was_available;
}

double* NetworkedDeviceQuatServer::getRotationQuaternion() {
	return quat_buffer;
}

double* NetworkedDeviceQuatServer::getGyroscope() {
	return gyro_buffer;
}

double* NetworkedDeviceQuatServer::getAccel() {
	return accel_buffer;
}

#define HELLOMESSAGE (" Hey OVR =D 5")

NetworkedDeviceQuatServer::NetworkedDeviceQuatServer(){
	quat_buffer = (double*)malloc(sizeof(double) * 4);
	gyro_buffer = (double*)malloc(sizeof(double) * 3);
	accel_buffer = (double*)malloc(sizeof(double) * 3);

	buff_hello = (char*)malloc(sizeof(HELLOMESSAGE));

	auto msg = HELLOMESSAGE;
	for (int i = 0; i < sizeof(HELLOMESSAGE); i++) {
		buff_hello[i] = msg[i];
	}
	buff_hello[0] = MSG_HANDSHAKE;

	buff_hello_len = sizeof(HELLOMESSAGE);
}
