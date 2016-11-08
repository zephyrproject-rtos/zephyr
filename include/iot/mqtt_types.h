/*
 * Copyright (c) 2016 Intel Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef _MQTT_TYPES_H_
#define _MQTT_TYPES_H_

#include <stdint.h>

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
	uint16_t client_id_len;	/* only required for unpacking */
	uint8_t will_flag:1;
	enum mqtt_qos will_qos;
	uint8_t will_retain:1;
	char *will_topic;
	uint16_t will_topic_len;
	uint8_t *will_msg;
	uint16_t will_msg_len;
	uint16_t keep_alive;
	const char *user_name;
	uint16_t user_name_len;	/*only required for unpacking */
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

#endif
