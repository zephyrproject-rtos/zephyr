/*
 * Copyright (c) 2016 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file	mqtt_pkt.h
 *
 * @brief	MQTT v3.1.1 packet library, see:
 *		http://docs.oasis-open.org/mqtt/mqtt/v3.1.1/mqtt-v3.1.1.html
 *
 *                This file is part of the Zephyr Project
 *                      http://www.zephyrproject.org
 */

#ifndef _MQTT_PKT_H_
#define _MQTT_PKT_H_

#include <stdint.h>
#include <stddef.h>

#include <net/mqtt_types.h>

#define MQTT_PACKET_TYPE(first_byte)	(((first_byte) & 0xF0) >> 4)

/**
 * Packs the MQTT CONNACK message. See MQTT 3.2 CONNACK - Acknowledge
 * connection request
 *
 * @param [out] buf Buffer where the resultant message is stored
 * @param [out] length Number of bytes required to codify the message
 * @param [in] size Buffer size
 * @param [in] session_present Session Present parameter (0 for clean session)
 * @param [in] ret_code Connect return code. See MQTT 3.2.2.3
 *
 * @retval 0 on success
 * @retval -ENOMEM if size < 4
 */
int mqtt_pack_connack(uint8_t *buf, uint16_t *length, uint16_t size,
		      uint8_t session_present, uint8_t ret_code);

/**
 * Packs the MQTT PUBACK message
 *
 * @param [out] buf Buffer where the resultant message is stored
 * @param [out] length Number of bytes required to codify the message
 * @param [in] size Buffer size
 * @param [in] pkt_id Packet Identifier. See MQTT 2.3.1 Packet Identifier
 *
 * @retval 0 on success
 * @retval -ENOMEM if size < 4
 */
int mqtt_pack_puback(uint8_t *buf, uint16_t *length, uint16_t size,
		     uint16_t pkt_id);

/**
 * Packs the MQTT PUBREC message
 *
 * @param [out] buf Buffer where the resultant message is stored
 * @param [out] length Number of bytes required to codify the message
 * @param [in] size Buffer size
 * @param [in] pkt_id Packet Identifier. See MQTT 2.3.1 Packet Identifier
 *
 * @retval 0 on success
 * @retval -ENOMEM if size < 4
 */
int mqtt_pack_pubrec(uint8_t *buf, uint16_t *length, uint16_t size,
		     uint16_t pkt_id);

/**
 * Packs the MQTT PUBREL message
 *
 * @param [out] buf Buffer where the resultant message is stored
 * @param [out] length Number of bytes required to codify the message
 * @param [in] size Buffer size
 * @param [in] pkt_id Packet Identifier. See MQTT 2.3.1 Packet Identifier
 *
 * @retval 0 on success
 * @retval -ENOMEM if size < 4
 */
int mqtt_pack_pubrel(uint8_t *buf, uint16_t *length, uint16_t size,
		     uint16_t pkt_id);

/**
 * Packs the MQTT PUBCOMP message
 *
 * @param [out] buf Buffer where the resultant message is stored
 * @param [out] length Number of bytes required to codify the message
 * @param [in] size Buffer size
 * @param [in] pkt_id Packet Identifier. See MQTT 2.3.1 Packet Identifier
 *
 * @retval 0 on success
 * @retval -ENOMEM if size < 4
 */
int mqtt_pack_pubcomp(uint8_t *buf, uint16_t *length, uint16_t size,
		      uint16_t pkt_id);

/**
 * Packs the MQTT SUBACK message
 *
 * @param [out] buf Buffer where the resultant message is stored
 * @param [out] length Number of bytes required to codify the message
 * @param [in] size Buffer size
 * @param [in] pkt_id Packet Identifier. See MQTT 2.3.1
 * @param [in] elements Number of elements in the granted_qos array
 * @param [in] granted_qos Array where the QoS values are stored
 *
 * @retval 0 on success
 * @retval -EINVAL
 * @retval -ENOMEM
 */
int mqtt_pack_suback(uint8_t *buf, uint16_t *length, uint16_t size,
		     uint16_t pkt_id, uint8_t elements,
		     enum mqtt_qos granted_qos[]);

/**
 * Packs the MQTT CONNECT message
 *
 * @param [out] buf Buffer where the resultant message is stored
 * @param [out] length Number of bytes required to codify the message
 * @param [in] size Buffer size
 * @param [in] msg MQTT CONNECT msg
 *
 * @retval 0 on success
 * @retval -EINVAL
 * @retval -ENOMEM
 */
int mqtt_pack_connect(uint8_t *buf, uint16_t *length, uint16_t size,
		      struct mqtt_connect_msg *msg);

/**
 * Unpacks the MQTT CONNECT message
 *
 * @param [in] buf Buffer where the message is stored
 * @param [in] length MQTT CONNECT message's length
 * @param [out] msg MQTT CONNECT parameters
 *
 * @retval 0 on success
 * @retval -EINVAL
 */
int mqtt_unpack_connect(uint8_t *buf, uint16_t length,
			struct mqtt_connect_msg *msg);

/**
 * Packs the MQTT SUBSCRIBE message
 *
 * @param [out] buf Buffer where the resultant message is stored
 * @param [out] length Number of bytes required to codify the message
 * @param [in] size Buffer size
 * @param [in] pkt_id MQTT Message Packet Identifier. See MQTT 2.3.1
 * @param [in] items Number of elements in topics
 * @param [in] topics Array of topics.
 *             For example: {"sensors", "lights", "doors"}
 * @param [in] qos Array of QoS values per topic.
 *                 For example: {MQTT_QoS1, MQTT_QoS2, MQTT_QoS0}
 *                 NOTE: qos and topics must have the same cardinality
 *                 (# of items)
 *
 * @retval 0 on success
 * @retval -EINVAL
 * @retval -ENOMEM
 */
int mqtt_pack_subscribe(uint8_t *buf, uint16_t *length, uint16_t size,
			uint16_t pkt_id, uint8_t items, const char *topics[],
			const enum mqtt_qos qos[]);

/**
 * Packs the MQTT UNSUBSCRIBE message
 *
 * @param [out] buf Buffer where the resultant message is stored
 * @param [out] length Number of bytes required to codify the message
 * @param [in] size Buffer size
 * @param [in] pkt_id MQTT Message Packet Identifier. See MQTT 2.3.1
 * @param [in] items Number of elements in topics
 * @param [in] topics Array of topics.
 *                    For example: {"sensors", "lights", "doors"}
 *
 * @retval 0 on success
 * @retval -EINVAL
 * @retval -ENOMEM
 */
int mqtt_pack_unsubscribe(uint8_t *buf, uint16_t *length, uint16_t size,
			  uint16_t pkt_id, uint8_t items, const char *topics[]);

/**
 * Unpacks the MQTT SUBSCRIBE message
 *
 * @param [in] buf Buffer where the message is stored
 * @param [in] length Message's length
 * @param [out] pkt_id MQTT Message Packet Identifier. See MQTT 2.3.1
 * @param [out] items Number of recovered topics
 * @param [in] elements Max number of topics to recover from buf
 * @param [out] topics Array of topics. Each element in this array points to
 *                     the beginning of a topic in buf
 * @param [out] topic_len Array containing the length of each topic in topics
 * @param [out] qos Array of QoS values
 *
 * @retval 0 on success
 * @retval -EINVAL
 */
int mqtt_unpack_subscribe(uint8_t *buf, uint16_t length, uint16_t *pkt_id,
			  uint8_t *items, uint8_t elements, char *topics[],
			  uint16_t topic_len[], enum mqtt_qos qos[]);

/**
 * Unpacks the MQTT SUBACK message
 *
 * @param [in] buf Buffer where the message is stored
 * @param [in] length Message's length
 * @param [out] pkt_id MQTT Message Packet Identifier. See MQTT 2.3.1
 * @param [out] items Number of recovered topics
 * @param [in] elements Max number of topics to recover from buf
 * @param [out] granted_qos Granted QoS values per topic. See MQTT 3.9
 *
 * @retval 0 on success
 * @retval -EINVAL
 */
int mqtt_unpack_suback(uint8_t *buf, uint16_t length, uint16_t *pkt_id,
		       uint8_t *items, uint8_t elements,
		       enum mqtt_qos granted_qos[]);

/**
 * Packs the MQTT PUBLISH message
 *
 * @param [out] buf Buffer where the resultant message is stored
 * @param [out] length Number of bytes required to codify the message
 * @param [in] size Buffer size
 * @param [in] msg MQTT PUBLISH message
 *
 * @retval 0 on success
 * @retval -EINVAL
 * @retval -ENOMEM
 */
int mqtt_pack_publish(uint8_t *buf, uint16_t *length, uint16_t size,
		      struct mqtt_publish_msg *msg);

/**
 * Unpacks the MQTT PUBLISH message
 *
 * @param [in] buf Buffer where the message is stored
 * @param [in] length Message's length
 * @param [out] msg MQTT PUBLISH message
 *
 * @retval 0 on success
 * @retval -EINVAL
 */
int mqtt_unpack_publish(uint8_t *buf, uint16_t length,
			struct mqtt_publish_msg *msg);

/**
 * Unpacks the MQTT CONNACK message
 *
 * @param [in] buf Buffer where the message is stored
 * @param [in] length Message's length
 * @param [out] session Session Present. See MQTT 3.2.2.2
 * @param [out] connect_rc CONNECT return code. See MQTT 3.2.2.3
 *
 * @retval 0 on success
 * @retval -EINVAL
 */
int mqtt_unpack_connack(uint8_t *buf, uint16_t length, uint8_t *session,
			uint8_t *connect_rc);

/**
 * Packs the MQTT PINGREQ message
 *
 * @param [out] buf Buffer where the resultant message is stored
 * @param [out] length Number of bytes required to codify the message
 * @param [in] size Buffer size
 *
 * @retval 0 on success
 * @retval -ENOMEM
 */
int mqtt_pack_pingreq(uint8_t *buf, uint16_t *length, uint16_t size);

/**
 * Packs the MQTT PINGRESP message
 *
 * @param [out] buf Buffer where the resultant message is stored
 * @param [out] length Number of bytes required to codify the message
 * @param [in] size Buffer size
 *
 * @retval 0 on success
 * @retval -ENOMEM
 */
int mqtt_pack_pingresp(uint8_t *buf, uint16_t *length, uint16_t size);

/**
 * Packs the MQTT DISCONNECT message
 *
 * @param [out] buf Buffer where the resultant message is stored
 * @param [out] length Number of bytes required to codify the message
 * @param [in] size Buffer size
 *
 * @retval 0 on success
 * @retval -ENOMEM
 */
int mqtt_pack_disconnect(uint8_t *buf, uint16_t *length, uint16_t size);

/**
 * Packs the MQTT UNSUBACK message
 *
 * @param [out] buf Buffer where the resultant message is stored
 * @param [out] length Number of bytes required to codify the message
 * @param [in] size Buffer size
 * @param [in] pkt_id Packet Identifier. See MQTT 2.3.1 Packet Identifier
 *
 * @retval 0 on success
 * @retval -ENOMEM if size < 4
 */
int mqtt_pack_unsuback(uint8_t *buf, uint16_t *length, uint16_t size,
		       uint16_t pkt_id);

/**
 * Unpacks the MQTT PUBACK message
 *
 * @param [in] buf Buffer where the message is stored
 * @param [in] length Message's length
 * @param [out] pkt_id Packet Identifier
 *
 * @retval 0 on success
 * @retval -EINVAL
 */
int mqtt_unpack_puback(uint8_t *buf, uint16_t length, uint16_t *pkt_id);

/**
 * Unpacks the MQTT PUBREC message
 *
 * @param [in] buf Buffer where the message is stored
 * @param [in] length Message's length
 * @param [out] pkt_id Packet Identifier
 *
 * @retval 0 on success
 * @retval -EINVAL
 */
int mqtt_unpack_pubrec(uint8_t *buf, uint16_t length, uint16_t *pkt_id);

/**
 * Unpacks the MQTT PUBREL message
 *
 * @param [in] buf Buffer where the message is stored
 * @param [in] length Message's length
 * @param [out] pkt_id Packet Identifier
 *
 * @retval 0 on success
 * @retval -EINVAL
 */
int mqtt_unpack_pubrel(uint8_t *buf, uint16_t length, uint16_t *pkt_id);

/**
 * Unpacks the MQTT PUBCOMP message
 *
 * @param [in] buf Buffer where the message is stored
 * @param [in] length Message's length
 * @param [out] pkt_id Packet Identifier
 *
 * @retval 0 on success
 * @retval -EINVAL
 */
int mqtt_unpack_pubcomp(uint8_t *buf, uint16_t length, uint16_t *pkt_id);

/**
 * Unpacks the MQTT UNSUBACK message
 *
 * @param [in] buf Buffer where the message is stored
 * @param [in] length Message's length
 * @param [out] pkt_id Packet Identifier
 *
 * @retval 0 on success
 * @retval -EINVAL
 */
int mqtt_unpack_unsuback(uint8_t *buf, uint16_t length, uint16_t *pkt_id);

/**
 * Unpacks the MQTT PINGREQ message
 *
 * @param [in] buf Buffer where the message is stored
 * @param [in] length Message's length
 *
 * @retval 0 on success
 * @retval -EINVAL
 */
int mqtt_unpack_pingreq(uint8_t *buf, uint16_t length);

/**
 * Unpacks the MQTT PINGRESP message
 *
 * @param [in] buf Buffer where the message is stored
 * @param [in] length Message's length
 *
 * @retval 0 on success
 * @retval -EINVAL
 */
int mqtt_unpack_pingresp(uint8_t *buf, uint16_t length);

/**
 * Unpacks the MQTT DISCONNECT message
 *
 * @param [in] buf Buffer where the message is stored
 * @param [in] length Message's length
 *
 * @retval 0 on success
 * @retval -EINVAL
 */
int mqtt_unpack_disconnect(uint8_t *buf, uint16_t length);

#endif
