/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/** @file mqtt_decoder.c
 *
 * @brief Decoder functions needed for decoding packets received from the
 *        broker.
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(net_mqtt_dec, CONFIG_MQTT_LOG_LEVEL);

#include "mqtt_internal.h"
#include "mqtt_os.h"

/**
 * @brief Unpacks unsigned 8 bit value from the buffer from the offset
 *        requested.
 *
 * @param[inout] buf A pointer to the buf_ctx structure containing current
 *                   buffer position.
 * @param[out] val Memory where the value is to be unpacked.
 *
 * @retval 0 if the procedure is successful.
 * @retval -EINVAL if the buffer would be exceeded during the read
 */
static int unpack_uint8(struct buf_ctx *buf, uint8_t *val)
{
	NET_DBG(">> cur:%p, end:%p", buf->cur, buf->end);

	if ((buf->end - buf->cur) < sizeof(uint8_t)) {
		return -EINVAL;
	}

	*val = *(buf->cur++);

	NET_DBG("<< val:%02x", *val);

	return 0;
}

/**
 * @brief Unpacks unsigned 16 bit value from the buffer from the offset
 *        requested.
 *
 * @param[inout] buf A pointer to the buf_ctx structure containing current
 *                   buffer position.
 * @param[out] val Memory where the value is to be unpacked.
 *
 * @retval 0 if the procedure is successful.
 * @retval -EINVAL if the buffer would be exceeded during the read
 */
static int unpack_uint16(struct buf_ctx *buf, uint16_t *val)
{
	NET_DBG(">> cur:%p, end:%p", buf->cur, buf->end);

	if ((buf->end - buf->cur) < sizeof(uint16_t)) {
		return -EINVAL;
	}

	*val = *(buf->cur++) << 8; /* MSB */
	*val |= *(buf->cur++); /* LSB */

	NET_DBG("<< val:%04x", *val);

	return 0;
}

/**
 * @brief Unpacks utf8 string from the buffer from the offset requested.
 *
 * @param[inout] buf A pointer to the buf_ctx structure containing current
 *                   buffer position.
 * @param[out] str Pointer to a string that will hold the string location
 *                 in the buffer.
 *
 * @retval 0 if the procedure is successful.
 * @retval -EINVAL if the buffer would be exceeded during the read
 */
static int unpack_utf8_str(struct buf_ctx *buf, struct mqtt_utf8 *str)
{
	uint16_t utf8_strlen;
	int err_code;

	NET_DBG(">> cur:%p, end:%p", buf->cur, buf->end);

	err_code = unpack_uint16(buf, &utf8_strlen);
	if (err_code != 0) {
		return err_code;
	}

	if ((buf->end - buf->cur) < utf8_strlen) {
		return -EINVAL;
	}

	str->size = utf8_strlen;
	/* Zero length UTF8 strings permitted. */
	if (utf8_strlen) {
		/* Point to right location in buffer. */
		str->utf8 = buf->cur;
		buf->cur += utf8_strlen;
	} else {
		str->utf8 = NULL;
	}

	NET_DBG("<< str_size:%08x", (uint32_t)GET_UT8STR_BUFFER_SIZE(str));

	return 0;
}

/**
 * @brief Unpacks binary string from the buffer from the offset requested.
 *
 * @param[in] length Binary string length.
 * @param[inout] buf A pointer to the buf_ctx structure containing current
 *                   buffer position.
 * @param[out] str Pointer to a binary string that will hold the binary string
 *                 location in the buffer.
 *
 * @retval 0 if the procedure is successful.
 * @retval -EINVAL if the buffer would be exceeded during the read
 */
static int unpack_data(uint32_t length, struct buf_ctx *buf,
		       struct mqtt_binstr *str)
{
	NET_DBG(">> cur:%p, end:%p", buf->cur, buf->end);

	if ((buf->end - buf->cur) < length) {
		return -EINVAL;
	}

	str->len = length;

	/* Zero length binary strings are permitted. */
	if (length > 0) {
		str->data = buf->cur;
		buf->cur += length;
	} else {
		str->data = NULL;
	}

	NET_DBG("<< bin len:%08x", GET_BINSTR_BUFFER_SIZE(str));

	return 0;
}

/**@brief Decode MQTT Packet Length in the MQTT fixed header.
 *
 * @param[inout] buf A pointer to the buf_ctx structure containing current
 *                   buffer position.
 * @param[out] length Length of variable header and payload in the
 *                              MQTT message.
 *
 * @retval 0 if the procedure is successful.
 * @retval -EINVAL if the length decoding would use more that 4 bytes.
 * @retval -EAGAIN if the buffer would be exceeded during the read.
 */
static int packet_length_decode(struct buf_ctx *buf, uint32_t *length)
{
	uint8_t shift = 0U;
	uint8_t bytes = 0U;

	*length = 0U;
	do {
		if (bytes >= MQTT_MAX_LENGTH_BYTES) {
			return -EINVAL;
		}

		if (buf->cur >= buf->end) {
			return -EAGAIN;
		}

		*length += ((uint32_t)*(buf->cur) & MQTT_LENGTH_VALUE_MASK)
								<< shift;
		shift += MQTT_LENGTH_SHIFT;
		bytes++;
	} while ((*(buf->cur++) & MQTT_LENGTH_CONTINUATION_BIT) != 0U);

	if (*length > MQTT_MAX_PAYLOAD_SIZE) {
		return -EINVAL;
	}

	NET_DBG("length:0x%08x", *length);

	return 0;
}

int fixed_header_decode(struct buf_ctx *buf, uint8_t *type_and_flags,
			uint32_t *length)
{
	int err_code;

	err_code = unpack_uint8(buf, type_and_flags);
	if (err_code != 0) {
		return err_code;
	}

	return packet_length_decode(buf, length);
}

int connect_ack_decode(const struct mqtt_client *client, struct buf_ctx *buf,
		       struct mqtt_connack_param *param)
{
	int err_code;
	uint8_t flags, ret_code;

	err_code = unpack_uint8(buf, &flags);
	if (err_code != 0) {
		return err_code;
	}

	err_code = unpack_uint8(buf, &ret_code);
	if (err_code != 0) {
		return err_code;
	}

	if (client->protocol_version == MQTT_VERSION_3_1_1) {
		param->session_present_flag =
			flags & MQTT_CONNACK_FLAG_SESSION_PRESENT;

		NET_DBG("[CID %p]: session_present_flag: %d", client,
			 param->session_present_flag);
	}

	param->return_code = (enum mqtt_conn_return_code)ret_code;

	return 0;
}

int publish_decode(uint8_t flags, uint32_t var_length, struct buf_ctx *buf,
		   struct mqtt_publish_param *param)
{
	int err_code;
	uint32_t var_header_length;

	param->dup_flag = flags & MQTT_HEADER_DUP_MASK;
	param->retain_flag = flags & MQTT_HEADER_RETAIN_MASK;
	param->message.topic.qos = ((flags & MQTT_HEADER_QOS_MASK) >> 1);

	err_code = unpack_utf8_str(buf, &param->message.topic.topic);
	if (err_code != 0) {
		return err_code;
	}

	var_header_length = param->message.topic.topic.size + sizeof(uint16_t);

	if (param->message.topic.qos > MQTT_QOS_0_AT_MOST_ONCE) {
		err_code = unpack_uint16(buf, &param->message_id);
		if (err_code != 0) {
			return err_code;
		}

		var_header_length += sizeof(uint16_t);
	}

	if (var_length < var_header_length) {
		NET_ERR("Corrupted PUBLISH message, header length (%u) larger "
			 "than total length (%u)", var_header_length,
			 var_length);
		return -EINVAL;
	}

	param->message.payload.data = NULL;
	param->message.payload.len = var_length - var_header_length;

	return 0;
}

int publish_ack_decode(struct buf_ctx *buf, struct mqtt_puback_param *param)
{
	return unpack_uint16(buf, &param->message_id);
}

int publish_receive_decode(struct buf_ctx *buf, struct mqtt_pubrec_param *param)
{
	return unpack_uint16(buf, &param->message_id);
}

int publish_release_decode(struct buf_ctx *buf, struct mqtt_pubrel_param *param)
{
	return unpack_uint16(buf, &param->message_id);
}

int publish_complete_decode(struct buf_ctx *buf,
			    struct mqtt_pubcomp_param *param)
{
	return unpack_uint16(buf, &param->message_id);
}

int subscribe_ack_decode(struct buf_ctx *buf, struct mqtt_suback_param *param)
{
	int err_code;

	err_code = unpack_uint16(buf, &param->message_id);
	if (err_code != 0) {
		return err_code;
	}

	return unpack_data(buf->end - buf->cur, buf, &param->return_codes);
}

int unsubscribe_ack_decode(struct buf_ctx *buf,
			   struct mqtt_unsuback_param *param)
{
	return unpack_uint16(buf, &param->message_id);
}
