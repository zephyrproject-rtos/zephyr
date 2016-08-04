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

#ifndef _MQTT_PKT_H_
#define _MQTT_PKT_H_

#include <stdint.h>
#include <stddef.h>

#include "app_buf.h"

/**
 * @brief MQTT Packet Type
 */
enum mqtt_packet {
	MQTT_CONNECT = 1,
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
 * @brief MQTT QoS
 */
enum mqtt_qos {
	MQTT_QoS0 = 0,
	MQTT_QoS1,
	MQTT_QoS2
};

/**
 * @brief MQTT protocol version
 *
 * @details For MQTT v3.1.1 use MQTT_311.
 */
enum mqtt_protocol {
	MQTT_31 = 3,
	MQTT_311
};

/**
 * @brief The mqtt_msg_t struct
 * @details Structure used for publish-related operations.
 */
struct mqtt_msg_t {
	struct app_buf_t topic;
	struct app_buf_t payload;
	enum mqtt_qos qos;
	int retained;
	uint16_t pkt_id;
	int dup;
};

/**
 * @brief MQTT_MSG_INIT		Initializes a MQTT Message structure with
 *				default values
 */
#define MQTT_MSG_INIT {	.topic = APP_BUF_INIT(NULL, 0, 0),		\
			.payload = APP_BUF_INIT(NULL, 0, 0),		\
			.qos = MQTT_QoS0,				\
			.retained = 0,					\
			.pkt_id = 0,					\
			.dup = 0}

/**
 * @brief struct mqtt_client_ctx_t	MQTT Client context
 * @details Structure used as a state variable for MQTT applications
 */
struct mqtt_client_ctx_t {

	const char *client_id;
	int clean_session;
	enum mqtt_protocol proto;

	const char *username;
	const char *pass;

	int will_enabled;
	const char *will_topic;
	const char *will_payload;
	enum mqtt_qos will_qos;
	int will_retained;

	int keep_alive;

	uint16_t pkt_id;
};

/**
 * @brief MQTT_CLIENT_CTX_INIT	Initializes a structure with default values
 * @param name			MQTT client name
 */
#define MQTT_CLIENT_CTX_INIT(name)  {	.client_id = name,		\
					.clean_session = 1,		\
					.proto = MQTT_311,		\
					.username = NULL,		\
					.pass = NULL,			\
					.will_enabled = 0,		\
					.will_topic = NULL,		\
					.will_payload = NULL,		\
					.will_qos = MQTT_QoS0,		\
					.will_retained = 0,		\
					.keep_alive = 0,		\
					.pkt_id = 0}

/**
 * @brief mqtt_msg_topic	Sets the topic of a PUBLIC message
 * @param msg			PUBLISH message
 * @param str			C-string
 * @return			0, always
 */
int mqtt_msg_topic(struct mqtt_msg_t *msg, char *str);

/**
 * @brief mqtt_msg_payload_str	Sets str as the msg's payload
 * @param msg			PUBLISH message
 * @param str			C-string
 * @return			0, always
 */
int mqtt_msg_payload_str(struct mqtt_msg_t *msg, char *str);

/**
 * @brief mqtt_next_pktid	Computes a new Packet Id
 * @param mqtt_ctx		MQTT Client context structure
 * @param pkt_id		Packet Id
 * @return			0, always
 */
int mqtt_next_pktid(struct mqtt_client_ctx_t *mqtt_ctx, uint16_t *pkt_id);

/**
 * @brief mqtt_pack_msg		Packs a PUBACK, PUBCOMP, PUBREC or PUBREL
 *				message
 * @param buf			Buffer where the message is stored
 * @param type			Packet type
 * @param pkt_id		Packet Identifier
 * @param dup			DUP flag
 * @return			0 on success
 *				-EINVAL on error
 */
int mqtt_pack_msg(struct app_buf_t *buf, enum mqtt_packet type, uint16_t pkt_id,
	      int dup);

/**
 * @brief mqtt_pack_subscribe	Packs a SUBSCRIBE message
 * @param buf			Buffer where the message is stored
 * @param dup			DUP flag
 * @param pkt_id		Packet Identifier
 * @param topic			Topic that the client will be subscribed to
 * @param qos			Quality of Service for this susbscription
 * @return			0 on success
 *				-EINVAL on error
 */
int mqtt_pack_subscribe(struct app_buf_t *buf, int dup, uint16_t pkt_id,
		       char *topic, enum mqtt_qos qos);

/**
 * @brief mqtt_pack_unsubscribe
				Packs a UNSUBSCRIBE message
 * @param buf			Buffer where the message is stored
 * @param dup			DUP flag
 * @param pkt_id		Packet Identifier
 * @param topic			Topic to unsubscribe from
 * @return			0 on success
 *				-EINVAL on error
 */
int mqtt_pack_unsubscribe(struct app_buf_t *buf, int dup, uint16_t pkt_id,
			  char *topic);

/**
 * @brief mqtt_unpack_suback	Unpacks a SUBACK message
 * @param buf			Buffer where the message is stored
 * @param pkt_id		Packet Identifier
 * @param granted_qos		Granted QoS for this subscription
 * @return			0 on success
 *				-EINVAL on error
 */
int mqtt_unpack_suback(struct app_buf_t *buf, uint16_t *pkt_id,
		       int *granted_qos);


/**
 * @brief mqtt_unpack_unsuback	Unpacks a UNSUBACK message
 * @param buf			Buffer where the message is stored
 * @param pkt_id		Packet Identifier
 * @return			0 on success
 *				-EINVAL on error
 */
int mqtt_unpack_unsuback(struct app_buf_t *buf, uint16_t *pkt_id);

/**
 * @brief mqtt_pack_connect	Packs a CONNECT message
 * @param buf			Buffer where the message is stored
 * @param mqtt_ctx		MQTT Client context structure
 * @return			0 on success
 *				-EINVAL on error
 */
int mqtt_pack_connect(struct app_buf_t *buf,
		      struct mqtt_client_ctx_t *mqtt_ctx);

/**
 * @brief mqtt_unpack_connack	Unpacks a CONNACK message
 * @param buf			Buffer where the message is stored
 * @param session		Clean session value
 * @param connect_ack		Connect ACK result
 * @return			0 on success,
 *				-EINVAL on error
 */
int mqtt_unpack_connack(struct app_buf_t *buf, int *session, int *connect_ack);

/**
 * @brief mqtt_pack_disconnect	Packs a DISCONNECT message
 * @param buf			Buffer where the message is stored
 * @return			0 on success
 *				-EINVAL on error
 */
int mqtt_pack_disconnect(struct app_buf_t *buf);

/**
 * @brief mqtt_unpack_ack	Unpacks any ACKNOWLEDGE message
 * @param buf			Buffer where the message is stored
 * @param pkt_type		Type of received ACK
 * @param dup			DUP flag value
 * @param rcv_pkt_id		Received Packet Identifier
 * @return			0 on success
 *				-EINVAL on error
 */
int mqtt_unpack_ack(struct app_buf_t *buf, uint8_t *pkt_type,
		    uint8_t *dup, uint16_t *rcv_pkt_id);

/**
 * @brief mqtt_pack_publish	Packs a PUBLISH message
 * @param buf			Buffer where the message is stored
 * @param msg			MQTT Message structure
 * @return			0 on success
 *				-EINVAL on error
 *
 */
int mqtt_pack_publish(struct app_buf_t *buf, struct mqtt_msg_t *msg);

/**
 * @brief mqtt_unpack_publish	Unpacks a PUBLISH message
 * @param buf			Buffer where the message is stored
 * @param msg			MQTT Message structure
 * @return			0 on success
 *				-EINVAL on error
 */
int mqtt_unpack_publish(struct app_buf_t *buf, struct mqtt_msg_t *msg);

/**
 * @brief mqtt_pack_pubrel	Packs a PUBREL message
 * @param buf			Buffer where the message is stored
 * @param dup			DUP flag
 * @param pkt_id		Packet Identifier
 * @return			0 on success
 *				-EINVAL on error
 */
int mqtt_pack_pubrel(struct app_buf_t *buf, int dup, uint16_t pkt_id);

/**
 * @brief mqtt_pack_pingreq	Packs a PINGREQ message
 * @param buf			Buffer where the message is stored
 * @return			0 on success
 *				-EINVAL on error
 */
int mqtt_pack_pingreq(struct app_buf_t *buf);


/**
 * @brief mqtt_pack_pingresp	Packs a PINGRESP message
 * @param buf			Buffer where the message is stored
 * @return			0 on success
 *				-EINVAL on error
 */
int mqtt_pack_pingresp(struct app_buf_t *buf);

#endif
