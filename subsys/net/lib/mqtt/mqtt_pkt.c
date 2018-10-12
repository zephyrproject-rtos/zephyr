/*
 * Copyright (c) 2016 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file	mqtt_pkt.c
 *
 * @brief	MQTT v3.1.1 packet library, see:
 *		http://docs.oasis-open.org/mqtt/mqtt/v3.1.1/mqtt-v3.1.1.html
 *
 *		   This file is part of the Zephyr Project
 *			http://www.zephyrproject.org
 */

#include "mqtt_pkt.h"
#include <net/net_ip.h>			/* for htons/ntohs */
#include <string.h>
#include <errno.h>

#define PACKET_TYPE_SIZE		1
#define REM_LEN_MIN_SIZE		1
#define ENCLENBUF_MAX_SIZE		4
#define CONNECT_VARIABLE_HDR_SIZE	10	/* See MQTT 3.1.1	*/
#define CONNECT_MIN_SIZE		(PACKET_TYPE_SIZE + REM_LEN_MIN_SIZE +\
					 CONNECT_VARIABLE_HDR_SIZE)
#define CONNACK_SIZE			4
#define CONNACK_REMLEN			2	/* See MQTT 3.2.1	*/
#define MSG_PKTID_ONLY_SIZE		4

/* Fixed header, reserved bits */
#define PUBREL_RESERVED			2	/* See MQTT 3.6.1	*/
#define PUBACK_RESERVED			0
#define PUBREC_RESERVED			0
#define PUBCOMP_RESERVED		0
#define UNSUBACK_RESERVED		0
#define UNSUBSCRIBE_RESERVED		0

#define INT_SIZE			2	/* See MQTT 1.5.2	*/
#define KEEP_ALIVE_SIZE			2	/* See MQTT 3.1.2.10	*/
#define PACKET_ID_SIZE			2	/* See MQTT 2.3.1	*/
#define QoS_SIZE			1
#define FLAGS_SIZE			1
#define SUBSCRIBE_RESERVED		0x02	/* See MQTT 3.8.1	*/
#define MSG_ZEROLEN_SIZE		2

#define TOPIC_STR_MIN_SIZE		1
#define TOPIC_MIN_SIZE			(INT_SIZE + TOPIC_STR_MIN_SIZE +\
					 QoS_SIZE)

/**
 * Enhanced strlen function
 *
 * @details This function is introduced to allow developers to pass null strings
 * to functions that compute the length of strings. strlen returns random values
 * for null arguments, so mqtt_strlen(NULL) is quite useful when optional
 * strings are allowed by mqtt-related functions. For example: username in the
 * MQTT CONNECT message is an optional parameter, so
 * connect(..., username = NULL, ...) will work fine without passing an
 * additional parameter like:
 *	connect(.., is_username_present, username, ...) or
 *	connect(.., username, username_len, ...).
 *
 * @param str C-string or NULL
 *
 * @retval 0 for NULL
 * @retval strlen otherwise
 */
static inline
u16_t mqtt_strlen(const char *str)
{
	if (str) {
		return (u16_t)strlen(str);
	}

	return 0;
}

/**
 * Computes the amount of bytes needed to codify the value stored in len.
 *
 * @param [out] size Amount of bytes required to codify the value stored in len
 * @param [in] len Value to be codified
 *
 * @retval 0 on success
 * @retval -EINVAL
 */
static
int compute_rlen_size(u16_t *size, u32_t len)
{
	if (len <= 127) {
		*size = 1;
	} else if (len >= 128 && len <= 16383) {
		*size = 2;
	} else if (len >= 16384 && len <= 2097151) {
		*size = 3;
	} else if (len >= 2097152 && len <= 268435455) {
		*size = 4;
	} else {
		return -EINVAL;
	}

	return 0;
}

/**
 * Remaining Length encoding algorithm. See MQTT 2.2.3 Remaining Length
 *
 * @param [out] buf Buffer where the encoded value will be stored
 * @param [in] len Value to encode
 *
 * @retval 0 always
 */
static int rlen_encode(u8_t *buf, u32_t len)
{
	u8_t encoded;
	u8_t i;

	i = 0;
	do {
		encoded = len % 128;
		len = len / 128;
		/* if there are more data to encode,
		 * set the top bit of this byte
		 */
		if (len > 0) {
			encoded = encoded | 128;
		}
		buf[i++] = encoded;
	} while (len > 0);

	return 0;
}

/**
 * Remaining Length decoding algorithm. See MQTT 2.2.3 Remaining Length
 *
 * @param [out] rlen Remaining Length (decoded)
 * @param [out] rlen_size Number of bytes required to codify rlen's value
 * @param [in] buf Buffer where the codified Remaining Length is codified
 * @param [in] size Buffer size
 *
 * @retval 0 on success
 * @retval -ENOMEM if size < 4
 */
static int rlen_decode(u32_t *rlen, u16_t *rlen_size,
		       u8_t *buf, u16_t size)
{
	u32_t value = 0;
	u32_t mult = 1;
	u16_t i = 0;
	u8_t encoded;

	do {
		if (i >= ENCLENBUF_MAX_SIZE || i >= size) {
			return -ENOMEM;
		}

		encoded = buf[i++];
		value += (encoded & 127) * mult;
		mult *= 128;
	} while ((encoded & 128) != 0);

	*rlen = value;
	*rlen_size = i;

	return 0;
}

int mqtt_pack_connack(u8_t *buf, u16_t *length, u16_t size,
		      u8_t session_present, u8_t ret_code)
{
	if (size < CONNACK_SIZE) {
		return -ENOMEM;
	}

	buf[0] = MQTT_CONNACK << 4;
	buf[1] = CONNACK_REMLEN;
	buf[2] = (session_present ? 0x01 : 0x00);
	buf[3] = ret_code & 0xFF;
	*length = CONNACK_SIZE;

	return 0;
}

/**
 * Packs a message that only contains the Packet Identifier as payload.
 *
 * @details Many MQTT messages only codify the packet type, reserved flags and
 * the Packet Identifier as payload, so this function is used by those MQTT
 * messages. The total size of this message is always 4 bytes, with a payload
 * of only 2 bytes to codify the identifier.
 *
 * @param [out] buf Buffer where the resultant message is stored
 * @param [out] length Number of bytes required to codify the message
 * @param [in] size Buffer's size
 * @param [in] type MQTT Control Packet type
 * @param [in] reserved	Control Packet Reserved Flags. See MQTT 2.2.2 Flags
 * @param [in] pkt_id Packet Identifier. See MQTT 2.3.1 Packet Identifier
 *
 * @retval 0 on success
 * @retval -ENOMEM if size < 4
 */
static
int pack_pkt_id(u8_t *buf, u16_t *length, u16_t size,
		enum mqtt_packet type, u8_t reserved, u16_t pkt_id)
{
	if (size < MSG_PKTID_ONLY_SIZE) {
		return -ENOMEM;
	}

	buf[0] = (type << 4) + (reserved & 0x0F);
	buf[1] = PACKET_ID_SIZE;
	UNALIGNED_PUT(htons(pkt_id), (u16_t *)(buf + PACKET_ID_SIZE));
	*length = MSG_PKTID_ONLY_SIZE;

	return 0;
}

int mqtt_pack_puback(u8_t *buf, u16_t *length, u16_t size,
		     u16_t pkt_id)
{
	return pack_pkt_id(buf, length, size, MQTT_PUBACK, 0, pkt_id);
}

int mqtt_pack_pubrec(u8_t *buf, u16_t *length, u16_t size,
		     u16_t pkt_id)
{
	return pack_pkt_id(buf, length, size, MQTT_PUBREC, 0, pkt_id);
}

int mqtt_pack_pubrel(u8_t *buf, u16_t *length, u16_t size,
		     u16_t pkt_id)
{
	return pack_pkt_id(buf, length, size, MQTT_PUBREL, PUBREL_RESERVED,
			   pkt_id);
}

int mqtt_pack_pubcomp(u8_t *buf, u16_t *length, u16_t size,
		      u16_t pkt_id)
{
	return pack_pkt_id(buf, length, size, MQTT_PUBCOMP, 0, pkt_id);
}

int mqtt_pack_unsuback(u8_t *buf, u16_t *length, u16_t size,
		       u16_t pkt_id)
{
	return pack_pkt_id(buf, length, size, MQTT_UNSUBACK, 0, pkt_id);
}

int mqtt_pack_suback(u8_t *buf, u16_t *length, u16_t size,
		     u16_t pkt_id, u8_t elements,
		     enum mqtt_qos granted_qos[])
{
	u16_t rlen_size;
	u16_t offset;
	u16_t rlen;
	u8_t i;
	int rc;


	rlen = PACKET_ID_SIZE + QoS_SIZE*elements;
	rc = compute_rlen_size(&rlen_size, rlen);
	if (rc != 0) {
		return -EINVAL;
	}

	*length = PACKET_TYPE_SIZE + rlen_size + rlen;
	if (*length > size) {
		return -ENOMEM;
	}

	buf[0] = MQTT_SUBACK << 4;

	rlen_encode(buf + PACKET_TYPE_SIZE, rlen);
	offset = PACKET_TYPE_SIZE + rlen_size;

	UNALIGNED_PUT(htons(pkt_id), (u16_t *)(buf + offset));
	offset += PACKET_ID_SIZE;

	for (i = 0; i < elements; i++) {
		buf[offset + i] = granted_qos[i];
	}

	return 0;
}

int mqtt_pack_connect(u8_t *buf, u16_t *length, u16_t size,
		      struct mqtt_connect_msg *msg)
{
	u16_t total_buf_size;
	u16_t rlen_size;
	u16_t pkt_size;
	u16_t offset;
	int rc;

	/* ----------- Payload size ----------- */

	/* client_id				*/
	pkt_size = INT_SIZE;
	pkt_size += msg->client_id_len;

	/* will flag - optional			*/
	if (msg->will_flag) {
		/* will topic			*/
		pkt_size += INT_SIZE;
		pkt_size += msg->will_topic_len;
		/* will message - binary	*/
		pkt_size += INT_SIZE;
		pkt_size += msg->will_msg_len;
	}

	/* user_name - UTF-8 - optional		*/
	if (msg->user_name) {
		pkt_size += INT_SIZE;
		pkt_size += msg->user_name_len;
	}

	/* password - binary - optional		*/
	if (msg->password) {
		pkt_size += INT_SIZE;
		pkt_size += msg->password_len;
	}

	/* Variable Header size			*/
	pkt_size += CONNECT_VARIABLE_HDR_SIZE;

	rc = compute_rlen_size(&rlen_size, pkt_size);
	if (rc != 0) {
		return -EINVAL;
	}

	/* 1 Byte for the MQTT Control Packet Type
	 * + Remaining length field size
	 * + Variable Header Size + Payload size
	 */
	total_buf_size = PACKET_TYPE_SIZE + rlen_size + pkt_size;
	if (total_buf_size > size) {
		return -ENOMEM;
	}

	buf[0] = MQTT_CONNECT << 4;

	rlen_encode(buf + PACKET_TYPE_SIZE, pkt_size);
	offset = PACKET_TYPE_SIZE + rlen_size;

	/* Variable Header size for MQTT CONNECT is 10 bytes.
	 * The following magic numbers are specified by the standard.
	 * See MQTT 3.1.2 Variable Header.
	 */
	buf[offset + 0] = 0x00;
	buf[offset + 1] = 0x04;
	buf[offset + 2] = 'M';
	buf[offset + 3] = 'Q';
	buf[offset + 4] = 'T';
	buf[offset + 5] = 'T';
	buf[offset + 6] = 0x04;
	/* connect flags */
	buf[offset + 7] = (msg->user_name ? 1 << 7 : 0) |
			  (msg->password_len ? 1 << 6 : 0) |
			  (msg->will_retain ? 1 << 5 : 0) |
			  ((msg->will_qos & 0x03) << 3) |
			  (msg->will_flag ? 1 << 2 : 0) |
			  (msg->clean_session ? 1 << 1 : 0);

	UNALIGNED_PUT(htons(msg->keep_alive), (u16_t *)(buf + offset + 8));
	offset += 8 + INT_SIZE;
	/* end of the CONNECT's Variable Header */

	/* Payload */
	UNALIGNED_PUT(htons(msg->client_id_len),
		      (u16_t *)(buf + offset));
	offset += INT_SIZE;
	memcpy(buf + offset, msg->client_id, msg->client_id_len);
	offset += msg->client_id_len;

	if (msg->will_flag) {
		UNALIGNED_PUT(htons(msg->will_topic_len),
			      (u16_t *)(buf + offset));
		offset += INT_SIZE;
		memcpy(buf + offset, msg->will_topic,
		       msg->will_topic_len);
		offset += msg->will_topic_len;

		UNALIGNED_PUT(htons(msg->will_msg_len),
			      (u16_t *)(buf + offset));
		offset += INT_SIZE;
		memcpy(buf + offset, msg->will_msg, msg->will_msg_len);
		offset += msg->will_msg_len;
	}

	if (msg->user_name) {
		UNALIGNED_PUT(htons(msg->user_name_len),
			      (u16_t *)(buf + offset));
		offset += INT_SIZE;
		memcpy(buf + offset, msg->user_name, msg->user_name_len);
		offset += msg->user_name_len;
	}

	if (msg->password) {
		UNALIGNED_PUT(htons(msg->password_len),
			      (u16_t *)(buf + offset));
		offset += INT_SIZE;
		memcpy(buf + offset, msg->password, msg->password_len);
		offset += msg->password_len;
	}

	*length = total_buf_size;

	return 0;
}

/**
 * Recovers the length and sets val to point to the beginning of the value
 *
 * @param [in] buf Buffer where the length and value are stored
 * @param [in] length Buffer's length
 * @param [out] val Pointer to the beginning of the value
 * @param [out] val_len Recovered value's length
 *
 * @retval 0 on success
 * @retval -EINVAL
 */
static
int recover_value_len(u8_t *buf, u16_t length, u8_t **val,
		      u16_t *val_len)
{
	u16_t val_u16;

	if (length < INT_SIZE) {
		return -EINVAL;
	}

	val_u16 = UNALIGNED_GET((u16_t *)buf);
	*val_len = ntohs(val_u16);

	/* malformed packet: avoid buffer overflows */
	if ((INT_SIZE + *val_len) > length) {
		return -EINVAL;
	}
	*val = buf + INT_SIZE;

	return 0;
}

int mqtt_unpack_connect(u8_t *buf, u16_t length,
			struct mqtt_connect_msg *msg)
{
	u8_t user_name_flag;
	u8_t password_flag;
	u16_t rlen_size;
	u16_t val_u16;
	u32_t rlen;
	u8_t offset;
	int rc;

	(void)memset(msg, 0x00, sizeof(struct mqtt_connect_msg));

	/* MQTT CONNECT packet min size, assuming no payload:
	 * packet type + min rem length size + var size len
	 *      1      +       1             +     10
	 */
	if (length < CONNECT_MIN_SIZE) {
		return -EINVAL;
	}

	if (buf[0] != (MQTT_CONNECT << 4)) {
		return -EINVAL;
	}

	rc = rlen_decode(&rlen, &rlen_size, buf + PACKET_TYPE_SIZE,
			 length - PACKET_TYPE_SIZE);
	if (rc != 0) {
		return rc;
	}

	/* header size + remaining length value + rm length size */
	if (PACKET_TYPE_SIZE + rlen + rlen_size > length) {
		return -EINVAL;
	}

	/* offset points to the protocol name length */
	offset = PACKET_TYPE_SIZE + rlen_size;

	/* protocol name length, fixed value */
	if (buf[offset + 0] != 0x00 || buf[offset + 1] != 0x04) {
		return -EINVAL;
	}

	offset += INT_SIZE;

	if (buf[offset + 0] != 'M' || buf[offset + 1] != 'Q' ||
	    buf[offset + 2] != 'T' || buf[offset + 3] != 'T' ||
	    buf[offset + 4] != 0x04) {
		return -EINVAL;
	}

	/* MQTT string size (4) + Protocol Level size (1) */
	offset += 5;

	/* validating "Connect Flag" bit 0, it must be 0! */
	if ((buf[offset] & 0x01) != 0) {
		return -EINVAL;
	}

	/* connection flags */
	user_name_flag = (buf[offset] & 0x80) ? 1 : 0;
	password_flag = (buf[offset] & 0x40) ? 1 : 0;
	msg->will_retain = (buf[offset] & 0x20) ? 1 : 0;
	msg->will_qos = (buf[offset] & 0x18) >> 3;
	msg->will_flag = (buf[offset] & 0x04) ? 1 : 0;
	msg->clean_session = (buf[offset] & 0x02) ? 1 : 0;

	offset += FLAGS_SIZE;

	val_u16 = UNALIGNED_GET((u16_t *)(buf + offset));
	msg->keep_alive = ntohs(val_u16);
	offset += KEEP_ALIVE_SIZE;

	rc = recover_value_len(buf + offset, length - offset,
			       (u8_t **)&msg->client_id,
			       &msg->client_id_len);
	if (rc != 0) {
		return -EINVAL;
	}

	offset += INT_SIZE + msg->client_id_len;

	if (msg->will_flag) {
		rc = recover_value_len(buf + offset, length - offset,
				       (u8_t **)&msg->will_topic,
				       &msg->will_topic_len);
		if (rc != 0) {
			return -EINVAL;
		}

		offset += INT_SIZE + msg->will_topic_len;

		rc = recover_value_len(buf + offset, length - offset,
				       &msg->will_msg,
				       &msg->will_msg_len);
		if (rc != 0) {
			return -EINVAL;
		}

		offset += INT_SIZE + msg->will_msg_len;
	}

	if (user_name_flag) {
		rc = recover_value_len(buf + offset, length - offset,
				       (u8_t **)&msg->user_name,
				       &msg->user_name_len);
		if (rc != 0) {
			return -EINVAL;
		}

		offset += INT_SIZE + msg->user_name_len;
	}

	if (password_flag) {
		rc = recover_value_len(buf + offset, length - offset,
				       &msg->password, &msg->password_len);
		if (rc != 0) {
			return -EINVAL;
		}
	}

	return 0;
}

/**
 * Computes the packet size for the SUBSCRIBE and UNSUBSCRIBE messages
 *
 * This routine does not consider the packet type field size (1 byte)
 *
 * @param rlen_size Remaining length size
 * @param payload_size SUBSCRIBE or UNSUBSCRIBE payload size
 * @param items Number of topics
 * @param topics Array of C-strings containing the topics to subscribe to
 * @param with_qos 0 for UNSUBSCRIBE, != 0 for SUBSCRIBE
 *
 * @retval 0 on success
 * @retval -EINVAL on error
 */
static
int subscribe_size(u16_t *rlen_size, u16_t *payload_size, u8_t items,
		   const char *topics[], enum mqtt_qos with_qos)
{
	u8_t i;
	int rc;

	*payload_size = PACKET_ID_SIZE;

	for (i = 0; i < items; i++) {
		/* topic length (as string) +1 byte for its QoS		*/
		*payload_size += mqtt_strlen(topics[i]) +
				 (with_qos ? QoS_SIZE : 0);
	}

	/* we add two bytes per topic to codify the topic's length	*/
	*payload_size += items * INT_SIZE;

	rc = compute_rlen_size(rlen_size, *payload_size);
	if (rc != 0) {
		return -EINVAL;
	}

	return 0;
}

/**
 * Packs the SUBSCRIBE and UNSUBSCRIBE messages
 *
 * @param buf Buffer where the message will be stored
 * @param pkt_id Packet Identifier
 * @param items Number of topics
 * @param topics Array of C-strings containing the topics to subscribe to
 * @param qos Array of QoS' values, qos[i] is the QoS of topic[i]
 * @param type MQTT_SUBSCRIBE or MQTT_UNSUBSCRIBE
 *
 * @retval 0 on success
 * @retval -EINVAL
 * @retval -ENOMEM
 */
static int mqtt_pack_subscribe_unsubscribe(u8_t *buf, u16_t *length,
					   u16_t size, u16_t pkt_id,
					   u8_t items, const char *topics[],
					   const enum mqtt_qos qos[],
					   enum mqtt_packet type)
{
	u16_t rlen_size;
	u16_t payload;
	u16_t offset;
	u8_t i;
	int rc;

	if (items <= 0) {
		return -EINVAL;
	}

	if (type != MQTT_SUBSCRIBE && type != MQTT_UNSUBSCRIBE) {
		return -EINVAL;
	}

	rc = subscribe_size(&rlen_size, &payload, items, topics,
			    type == MQTT_SUBSCRIBE ? 1 : 0);
	if (rc != 0) {
		return -EINVAL;
	}

	/* full packet size is:
	 * rem len size + payload + 1 byte for the packet type field size
	 */
	if ((rlen_size + payload + 1) > size) {
		return -ENOMEM;
	}

	/* after data length validations, we are ready...		*/
	buf[0] = (type << 4) + 0x02;

	/* compute and set the packet length, return code not evaluated
	 * because we previously called compute_rlen_size
	 */
	rlen_encode(buf + PACKET_TYPE_SIZE, payload);

	offset = PACKET_TYPE_SIZE + rlen_size;

	UNALIGNED_PUT(htons(pkt_id), (u16_t *)(buf + offset));
	offset += PACKET_ID_SIZE;

	for (i = 0; i < items; i++) {
		u16_t topic_len = mqtt_strlen(topics[i]);

		UNALIGNED_PUT(htons(topic_len), (u16_t *)(buf + offset));
		offset += INT_SIZE;

		memcpy(buf + offset, topics[i], topic_len);
		offset += topic_len;

		if (type == MQTT_SUBSCRIBE) {
			buf[offset] = qos[i] & 0x03;
			offset += QoS_SIZE;
		}
	}

	*length = offset;

	return 0;
}

int mqtt_pack_subscribe(u8_t *buf, u16_t *length, u16_t size,
			u16_t pkt_id, u8_t items, const char *topics[],
			const enum mqtt_qos qos[])
{

	return mqtt_pack_subscribe_unsubscribe(buf, length, size, pkt_id,
					       items, topics, qos,
					       MQTT_SUBSCRIBE);
}

int mqtt_pack_unsubscribe(u8_t *buf, u16_t *length, u16_t size,
			  u16_t pkt_id, u8_t items, const char *topics[])
{
	return mqtt_pack_subscribe_unsubscribe(buf, length, size, pkt_id,
					       items, topics, NULL,
					       MQTT_UNSUBSCRIBE);
}

int mqtt_unpack_subscribe(u8_t *buf, u16_t length, u16_t *pkt_id,
			  u8_t *items, u8_t elements, char *topics[],
			  u16_t topic_len[], enum mqtt_qos qos[])
{
	u16_t rmlen_size;
	u16_t val_u16;
	u32_t rmlen;
	u16_t offset;
	u8_t i;
	int rc;

	rc = rlen_decode(&rmlen, &rmlen_size, buf + PACKET_TYPE_SIZE,
			 length - PACKET_TYPE_SIZE);

	if (rc != 0) {
		return -EINVAL;
	}

	/* avoid buffer overflow due to malformed message		*/
	if ((PACKET_TYPE_SIZE + rmlen_size + rmlen) > length) {
		return -EINVAL;
	}

	/* MQTT-3.8.2 and MQTT-3.8.3-3: pkt_id and one topic filter	*/
	if (PACKET_ID_SIZE + TOPIC_MIN_SIZE > rmlen) {
		return -EINVAL;
	}

	/* MQTT-3.8.1:	SUBSCRIBE Fixed Header				*/
	if (buf[0] != (MQTT_SUBSCRIBE << 4 | SUBSCRIBE_RESERVED)) {
		return -EINVAL;
	}

	offset = PACKET_TYPE_SIZE + rmlen_size;

	val_u16 = UNALIGNED_GET((u16_t *)(buf + offset));
	*pkt_id = ntohs(val_u16);
	offset += PACKET_ID_SIZE;

	*items = 0;
	for (i = 0; i < elements; i++) {
		if ((offset + TOPIC_MIN_SIZE) > length) {
			return -EINVAL;
		}

		val_u16 = UNALIGNED_GET((u16_t *)(buf + offset));
		topic_len[i] = ntohs(val_u16);
		offset += INT_SIZE;
		/* invalid topic length found: malformed message	*/
		if ((offset + topic_len[i] + QoS_SIZE) > length) {
			return -EINVAL;
		}

		topics[i] = (char *)(buf + offset);
		offset += topic_len[i];
		qos[i] = *(buf + offset);
		offset += QoS_SIZE;

		*items += 1;

		if (offset == length) {
			return 0;
		}
	}

	return 0;
}

int mqtt_unpack_suback(u8_t *buf, u16_t length, u16_t *pkt_id,
		       u8_t *items, u8_t elements,
		       enum mqtt_qos granted_qos[])
{
	u16_t rlen_size;
	enum mqtt_qos qos;
	u16_t val_u16;
	u32_t rlen;
	u16_t offset;
	u8_t i;
	int rc;

	*pkt_id = 0;
	*items = 0;

	if (elements <= 0) {
		return -EINVAL;
	}

	if (buf[0] != MQTT_SUBACK << 4) {
		return -EINVAL;
	}

	rc = rlen_decode(&rlen, &rlen_size, buf + PACKET_TYPE_SIZE,
			 length - PACKET_TYPE_SIZE);
	if (rc != 0) {
		return -EINVAL;
	}

	/* header size + remaining length value + rm length size	*/
	if (PACKET_TYPE_SIZE + rlen + rlen_size > length) {
		return -EINVAL;
	}

	offset = PACKET_TYPE_SIZE + rlen_size;

	val_u16 = UNALIGNED_GET((u16_t *)(buf + offset));
	*pkt_id = ntohs(val_u16);
	offset += PACKET_ID_SIZE;

	*items = rlen - PACKET_ID_SIZE;

	/* no enough space to store the QoS				*/
	if (*items > elements) {
		return -EINVAL;
	}

	for (i = 0; i < *items; i++) {
		qos = *(buf + offset);
		if (qos < MQTT_QoS0 || qos > MQTT_QoS2) {
			return -EINVAL;
		}

		granted_qos[i] = qos;
		offset += QoS_SIZE;
	}

	return 0;
}

int mqtt_pack_publish(u8_t *buf, u16_t *length, u16_t size,
		      struct mqtt_publish_msg *msg)
{
	u16_t offset;
	u16_t rlen_size;
	u16_t payload;
	int rc;

	if (msg->qos < MQTT_QoS0 || msg->qos > MQTT_QoS2) {
		return -EINVAL;
	}

	/* Packet Identifier is only included if QoS > QoS0. See MQTT 3.3.2.2
	 * So, payload size is:
	 * topic length size + topic length + packet id + msg's size
	 */
	payload = INT_SIZE + msg->topic_len +
		  (msg->qos > MQTT_QoS0 ? PACKET_ID_SIZE : 0) + msg->msg_len;

	rc = compute_rlen_size(&rlen_size, payload);
	if (rc != 0) {
		return -EINVAL;
	}

	/* full packet size is:
	 * 1 byte for the packet type field size + rem len size + payload
	 */
	if (PACKET_TYPE_SIZE + rlen_size + payload > size) {
		return -ENOMEM;
	}

	buf[0] = (MQTT_PUBLISH << 4) | ((msg->dup ? 1 : 0) << 3) |
		 (msg->qos << 1) | (msg->retain ? 1 : 0);

	/* compute and set the packet length, return code not evaluated
	 * because we previously called compute_rlen_size
	 */
	rlen_encode(buf + PACKET_TYPE_SIZE, payload);

	offset = PACKET_TYPE_SIZE + rlen_size;
	UNALIGNED_PUT(htons(msg->topic_len), (u16_t *)(buf + offset));
	offset += INT_SIZE;

	memcpy(buf + offset, msg->topic, msg->topic_len);
	offset += msg->topic_len;

	/* packet id is only present for QoS 1 and 2			*/
	if (msg->qos > MQTT_QoS0) {
		UNALIGNED_PUT(htons(msg->pkt_id), (u16_t *)(buf + offset));
		offset += PACKET_ID_SIZE;
	}

	memcpy(buf + offset, msg->msg, msg->msg_len);
	offset += msg->msg_len;

	*length = offset;

	return 0;
}

int mqtt_unpack_publish(u8_t *buf, u16_t length,
			struct mqtt_publish_msg *msg)
{
	u16_t rmlen_size;
	u16_t val_u16;
	u16_t offset;
	u32_t rmlen;
	int rc;

	if (buf[0] >> 4 != MQTT_PUBLISH) {
		return -EINVAL;
	}

	msg->dup = (buf[0] & 0x08) >> 3;
	msg->qos = (buf[0] & 0x06) >> 1;
	msg->retain = buf[0] & 0x01;

	rc = rlen_decode(&rmlen, &rmlen_size, buf + PACKET_TYPE_SIZE,
			 length - PACKET_TYPE_SIZE);
	if (rc != 0) {
		return -EINVAL;
	}

	if ((PACKET_TYPE_SIZE + rmlen_size + rmlen) > length) {
		return -EINVAL;
	}

	offset = PACKET_TYPE_SIZE + rmlen_size;
	val_u16 = UNALIGNED_GET((u16_t *)(buf + offset));
	msg->topic_len = ntohs(val_u16);

	offset += INT_SIZE;
	if (offset + msg->topic_len > length) {
		return -EINVAL;
	}

	msg->topic = (char *)(buf + offset);
	offset += msg->topic_len;

	val_u16 = UNALIGNED_GET((u16_t *)(buf + offset));
	if (msg->qos == MQTT_QoS1 || msg->qos == MQTT_QoS2) {
		msg->pkt_id = ntohs(val_u16);
		offset += PACKET_ID_SIZE;
	} else {
		msg->pkt_id = 0;
	}

	msg->msg_len = length - offset;
	msg->msg = buf + offset;

	return 0;
}

int mqtt_unpack_connack(u8_t *buf, u16_t length, u8_t *session,
			u8_t *connect_rc)
{
	if (length < CONNACK_SIZE) {
		return -EINVAL;
	}

	if (buf[0] != (MQTT_CONNACK << 4) || buf[1] != 2) {
		return -EINVAL;
	}

	if (buf[2] > 1) {
		return -EINVAL;
	}

	*session = buf[2];
	*connect_rc = buf[3];

	return 0;
}

/**
 * Packs a zero length message
 *
 * @details This function packs MQTT messages with no payload. No validations
 * are made about the input arguments, besides 'size' that must be at least
 * 2 bytes
 *
 * @param [out] buf Buffer where the resultant message is stored
 * @param [out] length Number of bytes required to codify the message
 * @param [in] size Buffer size
 * @param [in] pkt_type MQTT Control Packet Type. See MQTT 2.2.1
 * @param [in] reserved Reserved bits. See MQTT 2.2
 *
 * @retval 0 on success
 * @retval -ENOMEM
 */
static
int pack_zerolen(u8_t *buf, u16_t *length, u16_t size,
		 enum mqtt_packet pkt_type, u8_t reserved)
{
	if (size < MSG_ZEROLEN_SIZE) {
		return -ENOMEM;
	}

	buf[0] = (pkt_type << 4) + (reserved & 0x0F);
	buf[1] = 0x00;
	*length = MSG_ZEROLEN_SIZE;

	return 0;
}

int mqtt_pack_pingreq(u8_t *buf, u16_t *length, u16_t size)
{
	return pack_zerolen(buf, length, size, MQTT_PINGREQ, 0x00);
}

int mqtt_pack_pingresp(u8_t *buf, u16_t *length, u16_t size)
{
	return pack_zerolen(buf, length, size, MQTT_PINGRESP, 0x00);
}

int mqtt_pack_disconnect(u8_t *buf, u16_t *length, u16_t size)
{
	return pack_zerolen(buf, length, size, MQTT_DISCONNECT, 0x00);
}

/**
 * Unpacks a MQTT message with a Packet Id as payload
 *
 * @param [in] buf Buffer where the message is stored
 * @param [in] length Message's length
 * @param [out] type MQTT Control Packet type
 * @param [out] reserved Reserved flags
 * @param [out] pkt_id Packet Identifier
 *
 * @retval 0 on success
 * @retval -EINVAL
 */
static
int unpack_pktid(u8_t *buf, u16_t length, enum mqtt_packet *type,
		 u8_t *reserved, u16_t *pkt_id)
{
	if (length < MSG_PKTID_ONLY_SIZE) {
		return -EINVAL;
	}

	if (buf[1] != PACKET_ID_SIZE) {
		return -EINVAL;
	}

	*type = buf[0] >> 4;
	*reserved = buf[0] & 0x0F;
	*pkt_id = ntohs(*(u16_t *)(buf + 2));

	return 0;
}

/**
 * Unpacks and validates a MQTT message containing a Packet Identifier
 *
 * @details The message codified in buf must contain a 1) packet type,
 * 2) reserved flags and 3) packet identifier. The user must provide the
 * expected packet type and expected reserved flags. See MQTT 2.2.2 Flags.
 * If the message contains different values for type and reserved flags
 * than the ones passed as arguments, the function will return -EINVAL
 *
 * @param [in] buf Buffer where the message is stored
 * @param [in] length Message's length
 * @param [out] pkt_id Packet Identifier
 * @param [in] expected_type Expected MQTT Control Packet type
 * @param [in] expected_reserv Expected Reserved Flags
 *
 * @retval 0 on success
 * @retval -EINVAL
 */
static
int unpack_pktid_validate(u8_t *buf, u16_t length, u16_t *pkt_id,
			  u8_t expected_type, u8_t expected_reserv)
{
	enum mqtt_packet type;
	u8_t reserved;
	int rc;

	rc = unpack_pktid(buf, length, &type, &reserved, pkt_id);
	if (rc != 0) {
		return rc;
	}

	if (type != expected_type || reserved != expected_reserv) {
		return -EINVAL;
	}

	return 0;
}

int mqtt_unpack_puback(u8_t *buf, u16_t length, u16_t *pkt_id)
{
	return unpack_pktid_validate(buf, length, pkt_id, MQTT_PUBACK,
				     PUBACK_RESERVED);
}

int mqtt_unpack_pubrec(u8_t *buf, u16_t length, u16_t *pkt_id)
{
	return unpack_pktid_validate(buf, length, pkt_id, MQTT_PUBREC,
				     PUBREC_RESERVED);
}

int mqtt_unpack_pubrel(u8_t *buf, u16_t length, u16_t *pkt_id)
{
	return unpack_pktid_validate(buf, length, pkt_id, MQTT_PUBREL,
				     PUBREL_RESERVED);
}

int mqtt_unpack_pubcomp(u8_t *buf, u16_t length, u16_t *pkt_id)
{
	return unpack_pktid_validate(buf, length, pkt_id, MQTT_PUBCOMP,
				     PUBCOMP_RESERVED);
}

int mqtt_unpack_unsuback(u8_t *buf, u16_t length, u16_t *pkt_id)
{
	return unpack_pktid_validate(buf, length, pkt_id, MQTT_UNSUBACK,
				     UNSUBACK_RESERVED);
}

/**
 * Unpacks a zero-length MQTT message
 *
 * @param [in] buf Buffer where the message is stored
 * @param [in] length Message's length
 * @param [out] pkt_type MQTT Control Packet type
 * @param [out] reserved Reserved flags
 *
 * @retval 0 on success
 * @retval -EINVAL
 */
static
int unpack_zerolen(u8_t *buf, u16_t length, enum mqtt_packet *pkt_type,
		   u8_t *reserved)
{
	if (length < MSG_ZEROLEN_SIZE) {
		return -EINVAL;
	}

	*pkt_type = buf[0] >> 4;
	*reserved = buf[0] & 0x0F;

	if (buf[1] != 0) {
		return -EINVAL;
	}

	return 0;
}

/**
 * Unpacks and validates a zero-len MQTT message
 *
 * @param [in] buf Buffer where the message is stored
 * @param [in] length Message's length
 * @param expected_type Expected MQTT Control Packet type
 * @param expected_reserved Expected Reserved Flags
 *
 * @retval 0 on success
 * @retval -EINVAL
 */
static
int unpack_zerolen_validate(u8_t *buf, u16_t length,
			    enum mqtt_packet expected_type,
			    u8_t expected_reserved)
{
	enum mqtt_packet pkt_type;
	u8_t reserved;
	int rc;

	rc = unpack_zerolen(buf, length, &pkt_type, &reserved);
	if (rc != 0) {
		return rc;
	}

	if (pkt_type != expected_type || reserved != expected_reserved) {
		return -EINVAL;
	}

	return 0;
}

int mqtt_unpack_pingreq(u8_t *buf, u16_t length)
{
	return unpack_zerolen_validate(buf, length, MQTT_PINGREQ, 0x00);
}

int mqtt_unpack_pingresp(u8_t *buf, u16_t length)
{
	return unpack_zerolen_validate(buf, length, MQTT_PINGRESP, 0x00);
}

int mqtt_unpack_disconnect(u8_t *buf, u16_t length)
{
	return unpack_zerolen_validate(buf, length, MQTT_DISCONNECT, 0x00);
}
