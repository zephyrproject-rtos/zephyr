/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _MQTT_TYPES_H_
#define _MQTT_TYPES_H_

#include <stdint.h>

/**
 * @brief MQTT library
 * @defgroup mqtt MQTT library
 * @{
 */

/**
 * @brief	MQTT Packet Type enum
 * @details	See MQTT 2.2.1: MQTT Control Packet type
 */
enum mqtt_packet {
	MQTT_INVALID = 0,
	MQTT_CONNECT,
	MQTT_CONNACK,
	MQTT_PUBLISH,
	MQTT_PUBACK,
	MQTT_PUBREC,
	MQTT_PUBREL,
	MQTT_PUBCOMP,
	MQTT_SUBSCRIBE,
	MQTT_SUBACK,
	MQTT_UNSUBSCRIBE,
	MQTT_UNSUBACK,
	MQTT_PINGREQ,
	MQTT_PINGRESP,
	MQTT_DISCONNECT
};

/**
 * @brief	MQTT protocol version
 */
enum mqtt_protocol {
	MQTT_311 = 4
};

/**
 * @brief	See MQTT 4.3 Quality of Service levels and protocol flows
 */
enum mqtt_qos {
	MQTT_QoS0 = 0,
	MQTT_QoS1,
	MQTT_QoS2
};

struct mqtt_connect_msg {
	uint8_t clean_session:1;
	char *client_id;
	uint16_t client_id_len;
	uint8_t will_flag:1;
	enum mqtt_qos will_qos;
	uint8_t will_retain:1;
	char *will_topic;
	uint16_t will_topic_len;
	uint8_t *will_msg;
	uint16_t will_msg_len;
	uint16_t keep_alive;
	const char *user_name;
	uint16_t user_name_len;
	uint8_t *password;
	uint16_t password_len;
};

struct mqtt_publish_msg {
	uint8_t dup;
	enum mqtt_qos qos;
	uint8_t retain;
	uint16_t pkt_id;
	char *topic;
	uint16_t topic_len;
	uint8_t *msg;
	uint16_t msg_len;
};

/**
 * @}
 */

#endif
