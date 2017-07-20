/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _MQTT_TYPES_H_
#define _MQTT_TYPES_H_

#include <zephyr/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @addtogroup mqtt
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
	u8_t clean_session:1;
	char *client_id;
	u16_t client_id_len;
	u8_t will_flag:1;
	enum mqtt_qos will_qos;
	u8_t will_retain:1;
	char *will_topic;
	u16_t will_topic_len;
	u8_t *will_msg;
	u16_t will_msg_len;
	u16_t keep_alive;
	const char *user_name;
	u16_t user_name_len;
	u8_t *password;
	u16_t password_len;
};

struct mqtt_publish_msg {
	u8_t dup;
	enum mqtt_qos qos;
	u8_t retain;
	u16_t pkt_id;
	char *topic;
	u16_t topic_len;
	u8_t *msg;
	u16_t msg_len;
};

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* _MQTT_TYPES_H_ */
