/*
 * Copyright (c) 2016 Intel Corporation.
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

/**
 * @file	mqtt_pkt.h
 *
 * @brief	MQTT v3.1.1 packet library, see:
 *		http://docs.oasis-open.org/mqtt/mqtt/v3.1.1/mqtt-v3.1.1.html
 *
 *		   This file is part of the Zephyr Project
 *			http://www.zephyrproject.org
 */

#ifndef _MQTT_PKT_H_
#define _MQTT_PKT_H_

#include <stdint.h>
#include <stddef.h>

#include <iot/mqtt_types.h>

/**
 * @brief mqtt_pack_connack	Packs the MQTT CONNACK message
 * @details			See MQTT 3.2 CONNACK - Acknowledge connection
 *				request
 * @param [out] buf		Buffer where the resultant message is stored
 * @param [out] length		Number of bytes required to codify the message
 * @param [in] size		Buffer's size
 * @param [in] session_present	Session Present parameter (0 for clean session)
 * @param [in] ret_code		Connect return code. See MQTT 3.2.2.3
 * @return			0 on success
 * @return			-ENOMEM if size < 4
 */
int mqtt_pack_connack(uint8_t *buf, uint16_t *length, uint16_t size,
		      uint8_t session_present, uint8_t ret_code);

/**
 * @brief mqtt_pack_puback	Packs the MQTT PUBACK message
 * @param [out] buf		Buffer where the resultant message is stored
 * @param [out] length		Number of bytes required to codify the message
 * @param [in] size		Buffer's size
 * @param [in] pkt_id		Packet Identifier. See MQTT 2.3.1 Packet
 *				Identifier
 * @return			0 on success
 * @return			-ENOMEM if size < 4
 */
int mqtt_pack_puback(uint8_t *buf, uint16_t *length, uint16_t size,
		     uint16_t pkt_id);

/**
 * @brief mqtt_pack_pubrec	Packs the MQTT PUBREC message
 * @param [out] buf		Buffer where the resultant message is stored
 * @param [out] length		Number of bytes required to codify the message
 * @param [in] size		Buffer's size
 * @param [in] pkt_id		Packet Identifier. See MQTT 2.3.1 Packet
 *				Identifier
 * @return			0 on success
 * @return			-ENOMEM if size < 4
 */
int mqtt_pack_pubrec(uint8_t *buf, uint16_t *length, uint16_t size,
		     uint16_t pkt_id);

/**
 * @brief mqtt_pack_puback	Packs the MQTT PUBREL message
 * @param [out] buf		Buffer where the resultant message is stored
 * @param [out] length		Number of bytes required to codify the message
 * @param [in] size		Buffer's size
 * @param [in] pkt_id		Packet Identifier. See MQTT 2.3.1 Packet
 *				Identifier
 * @return			0 on success
 * @return			-ENOMEM if size < 4
 */
int mqtt_pack_pubrel(uint8_t *buf, uint16_t *length, uint16_t size,
		     uint16_t pkt_id);

/**
 * @brief mqtt_pack_pubcomp	Packs the MQTT PUBCOMP message
 * @param [out] buf		Buffer where the resultant message is stored
 * @param [out] length		Number of bytes required to codify the message
 * @param [in] size		Buffer's size
 * @param [in] pkt_id		Packet Identifier. See MQTT 2.3.1 Packet
 *				Identifier
 * @return			0 on success
 * @return			-ENOMEM if size < 4
 */
int mqtt_pack_pubcomp(uint8_t *buf, uint16_t *length, uint16_t size,
		      uint16_t pkt_id);

/**
 * @brief mqtt_pack_suback	Packs the MQTT SUBACK message
 * @param [out] buf		Buffer where the resultant message is stored
 * @param [out] length		Number of bytes required to codify the message
 * @param [in] size		Buffer's size
 * @param [in] pkt_id		Packet Identifier. See MQTT 2.3.1
 * @param [in] elements		Number of elements in the granted_qos array
 * @param [in] granted_qos	Array where the QoS values are stored
 *				See MQTT 3.9 SUBACK, 3.9.3 Payload
 * @return			0 on success
 * @return			-EINVAL if an invalid parameter was passed
 *				        as an argument to this routine
 * @return			-ENOMEM if buf does not have enough reserved
 *					space to store the resultant message
 */
int mqtt_pack_suback(uint8_t *buf, uint16_t *length, uint16_t size,
		     uint16_t pkt_id, uint8_t elements,
		     enum mqtt_qos granted_qos[]);

/**
 * @brief mqtt_pack_connect	Packs the MQTT CONNECT message
 * @param [out] buf		Buffer where the resultant message is stored
 * @param [out] length		Number of bytes required to codify the message
 * @param [in] size		Buffer's size
 * @param [in] msg		MQTT CONNECT msg
 * @return			0 on success
 * @return			-EINVAL if an invalid parameter was passed
 *				        as an argument to this routine
 * @return			-ENOMEM if buf does not have enough reserved
 *					space to store the resultant message
 */
int mqtt_pack_connect(uint8_t *buf, uint16_t *length, uint16_t size,
		      struct mqtt_connect_msg *msg);

/**
 * @brief mqtt_unpack_connect	Unpacks the MQTT CONNECT message
 * @param [in] buf		Buffer where the message is stored
 * @param [in] length		MQTT CONNECT message's length
 * @param [out] msg		MQTT CONNECT parameters
 * @return			0 on success
 * @return			-EINVAL if an invalid parameter was passed
 *				        as an argument to this routine
 */
int mqtt_unpack_connect(uint8_t *buf, uint16_t length,
			struct mqtt_connect_msg *msg);

/**
 * @brief mqtt_pack_subscribe	Packs the MQTT SUBSCRIBE message
 * @param [out] buf		Buffer where the resultant message is stored
 * @param [out] length		Number of bytes required to codify the message
 * @param [in] size		Buffer's size
 * @param [in] pkt_id		MQTT Message Packet Identifier. See MQTT 2.3.1
 * @param [in] items		Number of elements in topics
 * @param [in] topics		Array of topics.
 *				For example: {"sensors", "lights", "doors"}
 * @param [in] qos		Array of QoS values per topic. For example:
				{MQTT_QoS1, MQTT_QoS2, MQTT_QoS0}
				NOTE: qos and topics must have the same
				cardinality (# of items)
 * @return			0 on success
 * @return			-EINVAL if an invalid parameter was passed
 *				        as an argument to this routine
 * @return			-ENOMEM if buf does not have enough reserved
 *					space to store the resultant message
 */
int mqtt_pack_subscribe(uint8_t *buf, uint16_t *length, uint16_t size,
			uint16_t pkt_id, uint8_t items, const char *topics[],
			enum mqtt_qos qos[]);

/**
 * @brief mqtt_pack_unsubscribe	Packs the MQTT UNSUBSCRIBE message
 * @param [out] buf		Buffer where the resultant message is stored
 * @param [out] length		Number of bytes required to codify the message
 * @param [in] size		Buffer's size
 * @param [in] pkt_id		MQTT Message Packet Identifier. See MQTT 2.3.1
 * @param [in] items		Number of elements in topics
 * @param [in] topics		Array of topics.
 *				For example: {"sensors", "lights", "doors"}
 * @return			0 on success
 * @return			-EINVAL if an invalid parameter was passed
 *				        as an argument to this routine
 * @return			-ENOMEM if buf does not have enough reserved
 *					space to store the resultant message
 */
int mqtt_pack_unsubscribe(uint8_t *buf, uint16_t *length, uint16_t size,
			  uint16_t pkt_id, uint8_t items, const char *topics[]);

/**
 * @brief mqtt_unpack_subscribe	Unpacks the MQTT SUBSCRIBE message
 * @param [in] buf		Buffer where the message is stored
 * @param [in] length		Message's length
 * @param [out] pkt_id		MQTT Message Packet Identifier. See MQTT 2.3.1
 * @param [out] items		Number of recovered topics
 * @param [in] elements		Max number of topics to recover from buf
 * @param [out] topics		Array of topics. Each element in this array
 *				points to the beginning of a topic in buf
 * @param [out] topic_len	Array of topics' length. Each element in this
 *				array contains the length of the
 * @param [out] qos		Array of QoS,
 * @return			0 on success
 * @return			-EINVAL if an invalid parameter was passed
 *				        as an argument to this routine
 */
int mqtt_unpack_subscribe(uint8_t *buf, uint16_t length, uint16_t *pkt_id,
			  uint8_t *items, uint8_t elements, char *topics[],
			  uint16_t topic_len[], enum mqtt_qos qos[]);

/**
 * @brief mqtt_unpack_suback	Unpacks the MQTT SUBACK message
 * @param [in] buf		Buffer where the message is stored
 * @param [in] length		Message's length
 * @param [out] pkt_id		MQTT Message Packet Identifier. See MQTT 2.3.1
 * @param [out] items		Number of recovered topics
 * @param [in] elements		Max number of topics to recover from buf
 * @param [out] granted_qos	Granted QoS values per topic. See MQTT 3.9
 * @return			0 on success
 * @return			-EINVAL if an invalid parameter was passed
 *				        as an argument to this routine
 */
int mqtt_unpack_suback(uint8_t *buf, uint16_t length, uint16_t *pkt_id,
		       uint8_t *items, uint8_t elements,
		       enum mqtt_qos granted_qos[]);

/**
 * @brief mqtt_pack_publish	Packs the MQTT PUBLISH message
 * @param [out] buf		Buffer where the resultant message is stored
 * @param [out] length		Number of bytes required to codify the message
 * @param [in] size		Buffer's size
 * @param [in] msg		MQTT PUBLISH message
 * @return			0 on success
 * @return			-EINVAL if an invalid parameter was passed
 *				        as an argument to this routine
 * @return			-ENOMEM if buf does not have enough reserved
 *					space to store the resultant message
 */
int mqtt_pack_publish(uint8_t *buf, uint16_t *length, uint16_t size,
		      struct mqtt_publish_msg *msg);

/**
 * @brief mqtt_unpack_publish	Unpacks the MQTT PUBLISH message
 * @param [in] buf		Buffer where the message is stored
 * @param [in] length		Message's length
 * @param [out] msg		MQTT PUBLISH message
 * @return			0 on success
 * @return			-EINVAL if an invalid parameter was passed
 *				        as an argument to this routine
 */
int mqtt_unpack_publish(uint8_t *buf, uint16_t length,
			struct mqtt_publish_msg *msg);

/**
 * @brief mqtt_unpack_connack	Unpacks the MQTT CONNACK message
 * @param [in] buf		Buffer where the message is stored
 * @param [in] length		Message's length
 * @param [out] session		Session Present. See MQTT 3.2.2.2
 * @param [out] connect_rc	CONNECT return code. See MQTT 3.2.2.3
 * @return			0 on success
 * @return			-EINVAL if an invalid parameter was passed
 *				        as an argument to this routine
 */
int mqtt_unpack_connack(uint8_t *buf, uint16_t length, uint8_t *session,
			uint8_t *connect_rc);

/**
 * @brief mqtt_pack_pingreq	Packs the MQTT PINGREQ message
 * @param [out] buf		Buffer where the resultant message is stored
 * @param [out] length		Number of bytes required to codify the message
 * @param [in] size		Buffer's size
 * @return			0 on success
 * @return			-ENOMEM if buf does not have enough reserved
 *					space to store the resultant message
 */
int mqtt_pack_pingreq(uint8_t *buf, uint16_t *length, uint16_t size);

/**
 * @brief mqtt_pack_pingresp	Packs the MQTT PINGRESP message
 * @param [out] buf		Buffer where the resultant message is stored
 * @param [out] length		Number of bytes required to codify the message
 * @param [in] size		Buffer's size
 * @return			0 on success
 * @return			-ENOMEM if buf does not have enough reserved
 *					space to store the resultant message
 */
int mqtt_pack_pingresp(uint8_t *buf, uint16_t *length, uint16_t size);

/**
 * @brief mqtt_pack_disconnect	Packs the MQTT DISCONNECT message
 * @param [out] buf		Buffer where the resultant message is stored
 * @param [out] length		Number of bytes required to codify the message
 * @param [in] size		Buffer's size
 * @return			0 on success
 * @return			-ENOMEM if buf does not have enough reserved
 *					space to store the resultant message
 */
int mqtt_pack_disconnect(uint8_t *buf, uint16_t *length, uint16_t size);

/**
 * @brief mqtt_pack_unsuback	Packs the MQTT UNSUBACK message
 * @param [out] buf		Buffer where the resultant message is stored
 * @param [out] length		Number of bytes required to codify the message
 * @param [in] size		Buffer's size
 * @param [in] pkt_id		Packet Identifier. See MQTT 2.3.1 Packet
 *				Identifier
 * @return			0 on success
 * @return			-ENOMEM if size < 4
 */
int mqtt_pack_unsuback(uint8_t *buf, uint16_t *length, uint16_t size,
		       uint16_t pkt_id);

/**
 * @brief mqtt_unpack_puback	Unpacks the MQTT PUBACK message
 * @param [in] buf		Buffer where the message is stored
 * @param [in] length		Message's length
 * @param [out] pkt_id		Packet Identifier
 * @return			0 on success
 * @return			-EINVAL if an invalid parameter was passed
 *				        as an argument to this routine or if
 *					buf does not contain the desired msg
 */
int mqtt_unpack_puback(uint8_t *buf, uint16_t length, uint16_t *pkt_id);

/**
 * @brief mqtt_unpack_pubrec	Unpacks the MQTT PUBREC message
 * @param [in] buf		Buffer where the message is stored
 * @param [in] length		Message's length
 * @param [out] pkt_id		Packet Identifier
 * @return			0 on success
 * @return			-EINVAL if an invalid parameter was passed
 *				        as an argument to this routine or if
 *					buf does not contain the desired msg
 */
int mqtt_unpack_pubrec(uint8_t *buf, uint16_t length, uint16_t *pkt_id);

/**
 * @brief mqtt_unpack_pubrel	Unpacks the MQTT PUBREL message
 * @param [in] buf		Buffer where the message is stored
 * @param [in] length		Message's length
 * @param [out] pkt_id		Packet Identifier
 * @return			0 on success
 * @return			-EINVAL if an invalid parameter was passed
 *				        as an argument to this routine or if
 *					buf does not contain the desired msg
 */
int mqtt_unpack_pubrel(uint8_t *buf, uint16_t length, uint16_t *pkt_id);

/**
 * @brief mqtt_unpack_pubcomp	Unpacks the MQTT PUBCOMP message
 * @param [in] buf		Buffer where the message is stored
 * @param [in] length		Message's length
 * @param [out] pkt_id		Packet Identifier
 * @return			0 on success
 * @return			-EINVAL if an invalid parameter was passed
 *				        as an argument to this routine or if
 *					buf does not contain the desired msg
 */
int mqtt_unpack_pubcomp(uint8_t *buf, uint16_t length, uint16_t *pkt_id);

/**
 * @brief mqtt_unpack_unsuback	Unpacks the MQTT UNSUBACK message
 * @param [in] buf		Buffer where the message is stored
 * @param [in] length		Message's length
 * @param [out] pkt_id		Packet Identifier
 * @return			0 on success
 * @return			-EINVAL if an invalid parameter was passed
 *				        as an argument to this routine or if
 *					buf does not contain the desired msg
 */
int mqtt_unpack_unsuback(uint8_t *buf, uint16_t length, uint16_t *pkt_id);

/**
 * @brief mqtt_unpack_pingreq	Unpacks the MQTT PINGREQ message
 * @param [in] buf		Buffer where the message is stored
 * @param [in] length		Message's length
 * @return			0 on success
 * @return			-EINVAL if an invalid parameter was passed
 *				        as an argument to this routine
 */
int mqtt_unpack_pingreq(uint8_t *buf, uint16_t length);

/**
 * @brief mqtt_unpack_pingresp	Unpacks the MQTT PINGRESP message
 * @param [in] buf		Buffer where the message is stored
 * @param [in] length		Message's length
 * @return			0 on success
 * @return			-EINVAL if an invalid parameter was passed
 *				        as an argument to this routine
 */
int mqtt_unpack_pingresp(uint8_t *buf, uint16_t length);

/**
 * @brief mqtt_unpack_disconnect Unpacks the MQTT DISCONNECT message
 * @param [in] buf		Buffer where the message is stored
 * @param [in] length		Message's length
 * @return			0 on success
 * @return			-EINVAL if an invalid parameter was passed
 *				        as an argument to this routine
 */
int mqtt_unpack_disconnect(uint8_t *buf, uint16_t length);

#endif
