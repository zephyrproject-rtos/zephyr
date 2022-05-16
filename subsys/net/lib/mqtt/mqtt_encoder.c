/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/** @file mqtt_encoder.c
 *
 * @brief Encoding functions needed to create packet to be sent to the broker.
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(net_mqtt_enc, CONFIG_MQTT_LOG_LEVEL);

#include "mqtt_internal.h"
#include "mqtt_os.h"

static const struct mqtt_utf8 mqtt_3_1_0_proto_desc =
	MQTT_UTF8_LITERAL("MQIsdp");

static const struct mqtt_utf8 mqtt_3_1_1_proto_desc =
	MQTT_UTF8_LITERAL("MQTT");

/** Never changing ping request, needed for Keep Alive. */
static const uint8_t ping_packet[MQTT_FIXED_HEADER_MIN_SIZE] = {
	MQTT_PKT_TYPE_PINGREQ,
	0x00
};

/** Never changing disconnect request. */
static const uint8_t disc_packet[MQTT_FIXED_HEADER_MIN_SIZE] = {
	MQTT_PKT_TYPE_DISCONNECT,
	0x00
};

/**
 * @brief Packs unsigned 8 bit value to the buffer at the offset requested.
 *
 * @param[in] val Value to be packed.
 * @param[inout] buf A pointer to the buf_ctx structure containing current
 *                   buffer position.
 *
 * @retval 0 if procedure is successful.
 * @retval -ENOMEM if there is no place in the buffer to store the value.
 */
static int pack_uint8(uint8_t val, struct buf_ctx *buf)
{
	if ((buf->end - buf->cur) < sizeof(uint8_t)) {
		return -ENOMEM;
	}

	NET_DBG(">> val:%02x cur:%p, end:%p", val, buf->cur, buf->end);

	/* Pack value. */
	*(buf->cur++) = val;

	return 0;
}

/**
 * @brief Packs unsigned 16 bit value to the buffer at the offset requested.
 *
 * @param[in] val Value to be packed.
 * @param[inout] buf A pointer to the buf_ctx structure containing current
 *                   buffer position.
 *
 * @retval 0 if the procedure is successful.
 * @retval -ENOMEM if there is no place in the buffer to store the value.
 */
static int pack_uint16(uint16_t val, struct buf_ctx *buf)
{
	if ((buf->end - buf->cur) < sizeof(uint16_t)) {
		return -ENOMEM;
	}

	NET_DBG(">> val:%04x cur:%p, end:%p", val, buf->cur, buf->end);

	/* Pack value. */
	*(buf->cur++) = (val >> 8) & 0xFF;
	*(buf->cur++) = val & 0xFF;

	return 0;
}

/**
 * @brief Packs utf8 string to the buffer at the offset requested.
 *
 * @param[in] str UTF-8 string and its length to be packed.
 * @param[inout] buf A pointer to the buf_ctx structure containing current
 *                   buffer position.
 *
 * @retval 0 if the procedure is successful.
 * @retval -ENOMEM if there is no place in the buffer to store the string.
 */
static int pack_utf8_str(const struct mqtt_utf8 *str, struct buf_ctx *buf)
{
	if ((buf->end - buf->cur) < GET_UT8STR_BUFFER_SIZE(str)) {
		return -ENOMEM;
	}

	NET_DBG(">> str_size:%08x cur:%p, end:%p",
		 (uint32_t)GET_UT8STR_BUFFER_SIZE(str), buf->cur, buf->end);

	/* Pack length followed by string. */
	(void)pack_uint16(str->size, buf);

	memcpy(buf->cur, str->utf8, str->size);
	buf->cur += str->size;

	return 0;
}

/**
 * @brief Computes and encodes length for the MQTT fixed header.
 *
 * @note The remaining length is not packed as a fixed unsigned 32 bit integer.
 *       Instead it is packed on algorithm below:
 *
 * @code
 * do
 *            encodedByte = X MOD 128
 *            X = X DIV 128
 *            // if there are more data to encode, set the top bit of this byte
 *            if ( X > 0 )
 *                encodedByte = encodedByte OR 128
 *            endif
 *                'output' encodedByte
 *       while ( X > 0 )
 * @endcode
 *
 * @param[in] length Length of variable header and payload in the MQTT message.
 * @param[inout] buf A pointer to the buf_ctx structure containing current
 *                   buffer position. May be NULL (in this case function will
 *                   only calculate number of bytes needed).
 *
 * @return Number of bytes needed to encode length value.
 */
static uint8_t packet_length_encode(uint32_t length, struct buf_ctx *buf)
{
	uint8_t encoded_bytes = 0U;

	NET_DBG(">> length:0x%08x cur:%p, end:%p", length,
		 (buf == NULL) ? 0 : buf->cur, (buf == NULL) ? 0 : buf->end);

	do {
		encoded_bytes++;

		if (buf != NULL) {
			*(buf->cur) = length & MQTT_LENGTH_VALUE_MASK;
		}

		length >>= MQTT_LENGTH_SHIFT;

		if (buf != NULL) {
			if (length > 0) {
				*(buf->cur) |= MQTT_LENGTH_CONTINUATION_BIT;
			}
			buf->cur++;
		}
	} while (length > 0);

	return encoded_bytes;
}

/**
 * @brief Encodes fixed header for the MQTT message and provides pointer to
 *        start of the header.
 *
 * @param[in] message_type Message type containing packet type and the flags.
 *                         Use @ref MQTT_MESSAGES_OPTIONS to construct the
 *                         message_type.
 * @param[in] start  Pointer to the start of the variable header.
 * @param[inout] buf Buffer context used to encode the frame.
 *                   The 5 bytes before the start of the message are assumed
 *                   by the routine to be available to pack the fixed header.
 *                   However, since the fixed header length is variable
 *                   length, the pointer to the start of the MQTT message
 *                   along with encoded fixed header is supplied as output
 *                   parameter if the procedure was successful.
 *                   As output, the pointers will point to beginning and the end
 *                   of the frame.
 *
 * @retval 0 if the procedure is successful.
 * @retval -EMSGSIZE if the message is too big for MQTT.
 */
static uint32_t mqtt_encode_fixed_header(uint8_t message_type, uint8_t *start,
				      struct buf_ctx *buf)
{
	uint32_t length = buf->cur - start;
	uint8_t fixed_header_length;

	if (length > MQTT_MAX_PAYLOAD_SIZE) {
		return -EMSGSIZE;
	}

	NET_DBG("<< msg type:0x%02x length:0x%08x", message_type, length);

	fixed_header_length = packet_length_encode(length, NULL);
	fixed_header_length += sizeof(uint8_t);

	NET_DBG("Fixed header length = %02x", fixed_header_length);

	/* Set the pointer at the start of the frame before encoding. */
	buf->cur = start - fixed_header_length;

	(void)pack_uint8(message_type, buf);
	(void)packet_length_encode(length, buf);

	/* Set the cur pointer back at the start of the frame,
	 * and end pointer to the end of the frame.
	 */
	buf->cur = buf->cur - fixed_header_length;
	buf->end = buf->cur + length + fixed_header_length;

	return 0;
}

/**
 * @brief Encodes a string of a zero length.
 *
 * @param[in] buffer_len Total size of the buffer on which string will be
 *                       encoded. This shall not be zero.
 * @param[inout] buf A pointer to the buf_ctx structure containing current
 *                   buffer position.
 *
 * @retval 0 if the procedure is successful.
 * @retval -ENOMEM if there is no place in the buffer to store the binary
 *                 string.
 */
static int zero_len_str_encode(struct buf_ctx *buf)
{
	return pack_uint16(0x0000, buf);
}

/**
 * @brief Encodes and sends messages that contain only message id in
 *        the variable header.
 *
 * @param[in] message_type Message type and reserved bit fields.
 * @param[in] message_id Message id to be encoded in the variable header.
 * @param[inout] buf_ctx Pointer to the buffer context structure,
 *                       containing buffer for the encoded message.
 *
 * @retval 0 or an error code indicating a reason for failure.
 */
static int mqtt_message_id_only_enc(uint8_t message_type, uint16_t message_id,
				    struct buf_ctx *buf)
{
	int err_code;
	uint8_t *start;

	/* Message id zero is not permitted by spec. */
	if (message_id == 0U) {
		return -EINVAL;
	}

	/* Reserve space for fixed header. */
	buf->cur += MQTT_FIXED_HEADER_MAX_SIZE;
	start = buf->cur;

	err_code = pack_uint16(message_id, buf);
	if (err_code != 0) {
		return err_code;
	}

	return mqtt_encode_fixed_header(message_type, start, buf);
}

int connect_request_encode(const struct mqtt_client *client,
			   struct buf_ctx *buf)
{
	uint8_t connect_flags = client->clean_session << 1;
	const uint8_t message_type =
			MQTT_MESSAGES_OPTIONS(MQTT_PKT_TYPE_CONNECT, 0, 0, 0);
	const struct mqtt_utf8 *mqtt_proto_desc;
	uint8_t *connect_flags_pos;
	int err_code;
	uint8_t *start;

	if (client->protocol_version == MQTT_VERSION_3_1_1) {
		mqtt_proto_desc = &mqtt_3_1_1_proto_desc;
	} else {
		mqtt_proto_desc = &mqtt_3_1_0_proto_desc;
	}

	/* Reserve space for fixed header. */
	buf->cur += MQTT_FIXED_HEADER_MAX_SIZE;
	start = buf->cur;

	NET_HEXDUMP_DBG(mqtt_proto_desc->utf8, mqtt_proto_desc->size,
			 "Encoding Protocol Description.");

	err_code = pack_utf8_str(mqtt_proto_desc, buf);
	if (err_code != 0) {
		return err_code;
	}

	NET_DBG("Encoding Protocol Version %02x.", client->protocol_version);
	err_code = pack_uint8(client->protocol_version, buf);
	if (err_code != 0) {
		return err_code;
	}

	/* Remember position of connect flag and leave one byte for it to
	 * be packed once we determine its value.
	 */
	connect_flags_pos = buf->cur;

	err_code = pack_uint8(0, buf);
	if (err_code != 0) {
		return err_code;
	}

	NET_DBG("Encoding Keep Alive Time %04x.", client->keepalive);
	err_code = pack_uint16(client->keepalive, buf);
	if (err_code != 0) {
		return err_code;
	}

	NET_HEXDUMP_DBG(client->client_id.utf8, client->client_id.size,
			 "Encoding Client Id.");
	err_code = pack_utf8_str(&client->client_id, buf);
	if (err_code != 0) {
		return err_code;
	}

	/* Pack will topic and QoS */
	if (client->will_topic != NULL) {
		connect_flags |= MQTT_CONNECT_FLAG_WILL_TOPIC;
		/* QoS is always 1 as of now. */
		connect_flags |= ((client->will_topic->qos & 0x03) << 3);
		connect_flags |= client->will_retain << 5;

		NET_HEXDUMP_DBG(client->will_topic->topic.utf8,
				 client->will_topic->topic.size,
				 "Encoding Will Topic.");
		err_code = pack_utf8_str(&client->will_topic->topic, buf);
		if (err_code != 0) {
			return err_code;
		}

		if (client->will_message != NULL) {
			NET_HEXDUMP_DBG(client->will_message->utf8,
					 client->will_message->size,
					 "Encoding Will Message.");
			err_code = pack_utf8_str(client->will_message, buf);
			if (err_code != 0) {
				return err_code;
			}
		} else {
			NET_DBG("Encoding Zero Length Will Message.");
			err_code = zero_len_str_encode(buf);
			if (err_code != 0) {
				return err_code;
			}
		}
	}

	/* Pack Username if any. */
	if (client->user_name != NULL) {
		connect_flags |= MQTT_CONNECT_FLAG_USERNAME;

		NET_HEXDUMP_DBG(client->user_name->utf8,
				 client->user_name->size,
				 "Encoding Username.");
		err_code = pack_utf8_str(client->user_name, buf);
		if (err_code != 0) {
			return err_code;
		}
	}

	/* Pack Password if any. */
	if (client->password != NULL) {
		connect_flags |= MQTT_CONNECT_FLAG_PASSWORD;

		NET_HEXDUMP_DBG(client->password->utf8,
				 client->password->size,
				 "Encoding Password.");
		err_code = pack_utf8_str(client->password, buf);
		if (err_code != 0) {
			return err_code;
		}
	}

	/* Write the flags the connect flags. */
	*connect_flags_pos = connect_flags;

	return mqtt_encode_fixed_header(message_type, start, buf);
}

int publish_encode(const struct mqtt_publish_param *param, struct buf_ctx *buf)
{
	const uint8_t message_type = MQTT_MESSAGES_OPTIONS(
			MQTT_PKT_TYPE_PUBLISH, param->dup_flag,
			param->message.topic.qos, param->retain_flag);
	int err_code;
	uint8_t *start;

	/* Message id zero is not permitted by spec. */
	if ((param->message.topic.qos) && (param->message_id == 0U)) {
		return -EINVAL;
	}

	/* Reserve space for fixed header. */
	buf->cur += MQTT_FIXED_HEADER_MAX_SIZE;
	start = buf->cur;

	err_code = pack_utf8_str(&param->message.topic.topic, buf);
	if (err_code != 0) {
		return err_code;
	}

	if (param->message.topic.qos) {
		err_code = pack_uint16(param->message_id, buf);
		if (err_code != 0) {
			return err_code;
		}
	}

	/* Do not copy payload. We move the buffer pointer to ensure that
	 * message length in fixed header is encoded correctly.
	 */
	buf->cur += param->message.payload.len;

	err_code = mqtt_encode_fixed_header(message_type, start, buf);
	if (err_code != 0) {
		return err_code;
	}

	buf->end -= param->message.payload.len;

	return 0;
}

int publish_ack_encode(const struct mqtt_puback_param *param,
		       struct buf_ctx *buf)
{
	const uint8_t message_type =
		MQTT_MESSAGES_OPTIONS(MQTT_PKT_TYPE_PUBACK, 0, 0, 0);

	return mqtt_message_id_only_enc(message_type, param->message_id, buf);
}

int publish_receive_encode(const struct mqtt_pubrec_param *param,
			   struct buf_ctx *buf)
{
	const uint8_t message_type =
		MQTT_MESSAGES_OPTIONS(MQTT_PKT_TYPE_PUBREC, 0, 0, 0);

	return mqtt_message_id_only_enc(message_type, param->message_id, buf);
}

int publish_release_encode(const struct mqtt_pubrel_param *param,
			   struct buf_ctx *buf)
{
	const uint8_t message_type =
		MQTT_MESSAGES_OPTIONS(MQTT_PKT_TYPE_PUBREL, 0, 1, 0);

	return mqtt_message_id_only_enc(message_type, param->message_id, buf);
}

int publish_complete_encode(const struct mqtt_pubcomp_param *param,
			    struct buf_ctx *buf)
{
	const uint8_t message_type =
		MQTT_MESSAGES_OPTIONS(MQTT_PKT_TYPE_PUBCOMP, 0, 0, 0);

	return mqtt_message_id_only_enc(message_type, param->message_id, buf);
}

int disconnect_encode(struct buf_ctx *buf)
{
	if (buf->end - buf->cur < sizeof(disc_packet)) {
		return -ENOMEM;
	}

	memcpy(buf->cur, disc_packet, sizeof(disc_packet));
	buf->end = buf->cur + sizeof(disc_packet);

	return 0;
}

int subscribe_encode(const struct mqtt_subscription_list *param,
		     struct buf_ctx *buf)
{
	const uint8_t message_type = MQTT_MESSAGES_OPTIONS(
			MQTT_PKT_TYPE_SUBSCRIBE, 0, 1, 0);
	int err_code, i;
	uint8_t *start;

	/* Message id zero is not permitted by spec. */
	if (param->message_id == 0U) {
		return -EINVAL;
	}

	/* Reserve space for fixed header. */
	buf->cur += MQTT_FIXED_HEADER_MAX_SIZE;
	start = buf->cur;

	err_code = pack_uint16(param->message_id, buf);
	if (err_code != 0) {
		return err_code;
	}

	for (i = 0; i < param->list_count; i++) {
		err_code = pack_utf8_str(&param->list[i].topic, buf);
		if (err_code != 0) {
			return err_code;
		}

		err_code = pack_uint8(param->list[i].qos, buf);
		if (err_code != 0) {
			return err_code;
		}
	}

	return mqtt_encode_fixed_header(message_type, start, buf);
}

int unsubscribe_encode(const struct mqtt_subscription_list *param,
		       struct buf_ctx *buf)
{
	const uint8_t message_type = MQTT_MESSAGES_OPTIONS(
		MQTT_PKT_TYPE_UNSUBSCRIBE, 0, MQTT_QOS_1_AT_LEAST_ONCE, 0);
	int err_code, i;
	uint8_t *start;

	/* Reserve space for fixed header. */
	buf->cur += MQTT_FIXED_HEADER_MAX_SIZE;
	start = buf->cur;

	err_code = pack_uint16(param->message_id, buf);
	if (err_code != 0) {
		return err_code;
	}

	for (i = 0; i < param->list_count; i++) {
		err_code = pack_utf8_str(&param->list[i].topic, buf);
		if (err_code != 0) {
			return err_code;
		}
	}

	return mqtt_encode_fixed_header(message_type, start, buf);
}

int ping_request_encode(struct buf_ctx *buf)
{
	if (buf->end - buf->cur < sizeof(ping_packet)) {
		return -ENOMEM;
	}

	memcpy(buf->cur, ping_packet, sizeof(ping_packet));
	buf->end = buf->cur + sizeof(ping_packet);

	return 0;
}
