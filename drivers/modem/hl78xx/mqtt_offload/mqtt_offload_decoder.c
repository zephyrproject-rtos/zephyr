/*
 * Copyright (c) 2025 Netfeasa Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/** @file mqtt_offload_decoder.c
 *
 * @brief HL78XX-MQTT message decoder.
 */

#include "mqtt_offload_msg.h"

#include <zephyr/net/mqtt_offload.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(net_mqtt_offload_decoder, CONFIG_MQTT_OFFLOAD_LOG_LEVEL);

/**
 * @brief Decode the length of a message payload.
 *
 * From the specification:
 *
 * The Length field is either 1- or 3-octet long and specifies the total number of octets
 * contained in the message (including the Length field itself).
 * If the first octet of the Length field is coded “0x01” then the Length field is 3-octet long;
 * in this case, the two following octets specify the total number of octets of the message
 * (most-significant octet first). Otherwise, the Length field is only 1-octet long and specifies
 * itself the total number of octets contained in the message. The 3-octet format allows the
 * encoding of message lengths up to 65535 octets. Messages with lengths smaller than 256 octets
 * may use the shorter 1-octet format.
 *
 * @param buf
 * @return Size of the message not including the length field or negative error code
 */
static ssize_t decode_payload_length(struct net_buf_simple *buf)
{
	size_t length;
	size_t length_field_s = 1;
	size_t buflen = buf->len;

	/*
	 * Size must not be larger than an uint16_t can fit,
	 * minus 3 bytes for the length field itself
	 */
	if (buflen > UINT16_MAX) {
		LOG_ERR("Message too large");
		return -EFBIG;
	}

	length = net_buf_simple_pull_u8(buf);
	if (length == MQTT_OFFLOAD_LENGTH_FIELD_EXTENDED_PREFIX) {
		length = net_buf_simple_pull_be16(buf);
		length_field_s = 3;
	}

	if (length != buflen) {
		LOG_ERR("Message length %zu != buffer size %zu", length, buflen);
		return -EPROTO;
	}

	if (length <= length_field_s) {
		LOG_ERR("Message length %zu - contains no data?", length);
		return -ENODATA;
	}

	/* subtract the size of the length field to get message length */
	return length - length_field_s;
}

static void decode_flags(struct net_buf_simple *buf, struct mqtt_offload_flags *flags)
{
	uint8_t b = net_buf_simple_pull_u8(buf);

	flags->retain = (bool)(b & MQTT_OFFLOAD_FLAGS_RETAIN);
	flags->will = (bool)(b & MQTT_OFFLOAD_FLAGS_WILL);
	flags->clean_session = (bool)(b & MQTT_OFFLOAD_FLAGS_CLEANSESSION);

	flags->qos = (enum mqtt_offload_qos)((b & MQTT_OFFLOAD_FLAGS_MASK_QOS) >>
					     MQTT_OFFLOAD_FLAGS_SHIFT_QOS);
}

static void decode_topic_name(struct net_buf_simple *buf, struct mqtt_offload_data *topic_name)
{
	topic_name->size = net_buf_simple_pull_be16(buf);
	topic_name->data = net_buf_simple_pull_mem(buf, topic_name->size);
	LOG_HEXDUMP_DBG(topic_name->data, topic_name->size, "Decoded HL78XX-MQTT topic");
}

static void decode_data(struct net_buf_simple *buf, struct mqtt_offload_data *dest)
{
	dest->size = buf->len;
	dest->data = net_buf_simple_pull_mem(buf, buf->len);
	LOG_HEXDUMP_DBG(dest->data, dest->size, "Decoded HL78XX-MQTT data");
}

static int decode_empty_message(struct net_buf_simple *buf)
{
	if (buf->len) {
		LOG_ERR("Message not empty");
		return -EPROTO;
	}

	return 0;
}

static int decode_msg_connack(struct net_buf_simple *buf, struct mqtt_offload_param_connack *params)
{
	if (buf->len != 1) {
		return -EPROTO;
	}

	params->ret_code = net_buf_simple_pull_u8(buf);

	return 0;
}

static int decode_msg_willtopicreq(struct net_buf_simple *buf)
{
	return decode_empty_message(buf);
}

static int decode_msg_willmsgreq(struct net_buf_simple *buf)
{
	return decode_empty_message(buf);
}

static int decode_msg_publish(struct net_buf_simple *buf, struct mqtt_offload_param_publish *params)
{
	struct mqtt_offload_flags flags;

	if (buf->len < 6) {
		return -EPROTO;
	}

	decode_flags(buf, &flags);
	params->qos = flags.qos;
	params->retain = flags.retain;
	decode_topic_name(buf, &params->topic);
	decode_data(buf, &params->data);

	return 0;
}

static int decode_msg_puback(struct net_buf_simple *buf, struct mqtt_offload_param_puback *params)
{
	if (buf->len != 3) {
		return -EPROTO;
	}

	params->msg_id = net_buf_simple_pull_be16(buf);
	params->ret_code = net_buf_simple_pull_u8(buf);

	return 0;
}

static int decode_msg_suback(struct net_buf_simple *buf, struct mqtt_offload_param_suback *params)
{
	struct mqtt_offload_flags flags;

	if (buf->len != 6) {
		return -EPROTO;
	}

	decode_flags(buf, &flags);

	params->qos = flags.qos;

	params->topic_id = net_buf_simple_pull_be16(buf);
	params->msg_id = net_buf_simple_pull_be16(buf);
	params->ret_code = net_buf_simple_pull_u8(buf);

	return 0;
}

static int decode_msg_unsuback(struct net_buf_simple *buf,
			       struct mqtt_offload_param_unsuback *params)
{
	if (buf->len != 2) {
		return -EPROTO;
	}

	params->msg_id = net_buf_simple_pull_be16(buf);

	return 0;
}

static int decode_msg_disconnect(struct net_buf_simple *buf,
				 struct mqtt_offload_param_disconnect *params)
{
	/* The duration field is only set if the message was sent by a client. */
	return decode_empty_message(buf);
}

static int decode_msg_willtopicresp(struct net_buf_simple *buf,
				    struct mqtt_offload_param_willtopicresp *params)
{
	if (buf->len != 1) {
		return -EPROTO;
	}

	params->ret_code = net_buf_simple_pull_u8(buf);

	return 0;
}

static int decode_msg_willmsgresp(struct net_buf_simple *buf,
				  struct mqtt_offload_param_willmsgresp *params)
{
	if (buf->len != 1) {
		return -EPROTO;
	}

	params->ret_code = net_buf_simple_pull_u8(buf);

	return 0;
}

int mqtt_offload_decode_msg(struct net_buf_simple *buf, struct mqtt_offload_param *params)
{
	ssize_t len;

	if (!buf || !buf->len) {
		return -EINVAL;
	}

	len = decode_payload_length(buf);
	if (len < 0) {
		LOG_ERR("Could not decode message: %d", (int)len);
		return (int)len;
	}

	params->type = (enum mqtt_offload_msg_type)net_buf_simple_pull_u8(buf);

	LOG_INF("Decoding message type: %d", params->type);

	switch (params->type) {
	case MQTT_OFFLOAD_MSG_TYPE_CONNACK:
		return decode_msg_connack(buf, &params->params.connack);
	case MQTT_OFFLOAD_MSG_TYPE_WILLTOPICREQ:
		return decode_msg_willtopicreq(buf);
	case MQTT_OFFLOAD_MSG_TYPE_WILLMSGREQ:
		return decode_msg_willmsgreq(buf);
	case MQTT_OFFLOAD_MSG_TYPE_PUBLISH:
		return decode_msg_publish(buf, &params->params.publish);
	case MQTT_OFFLOAD_MSG_TYPE_PUBACK:
		return decode_msg_puback(buf, &params->params.puback);
	case MQTT_OFFLOAD_MSG_TYPE_SUBACK:
		return decode_msg_suback(buf, &params->params.suback);
	case MQTT_OFFLOAD_MSG_TYPE_UNSUBACK:
		return decode_msg_unsuback(buf, &params->params.unsuback);
	case MQTT_OFFLOAD_MSG_TYPE_DISCONNECT:
		return decode_msg_disconnect(buf, &params->params.disconnect);
	case MQTT_OFFLOAD_MSG_TYPE_WILLTOPICRESP:
		return decode_msg_willtopicresp(buf, &params->params.willtopicresp);
	case MQTT_OFFLOAD_MSG_TYPE_WILLMSGRESP:
		return decode_msg_willmsgresp(buf, &params->params.willmsgresp);
	default:
		LOG_ERR("Got unexpected message type %d", params->type);
		return -EINVAL;
	}
}
