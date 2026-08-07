/*
 * Copyright (c) 2022 René Beckmann
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/** @file mqtt_sn_decoder.c
 *
 * @brief MQTT-SN message decoder.
 */

#include "mqtt_sn_msg.h"

#include <zephyr/net/mqtt_sn.h>

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(net_mqtt_sn, CONFIG_MQTT_SN_LOG_LEVEL);

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
	if (length == MQTT_SN_LENGTH_FIELD_EXTENDED_PREFIX) {
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

static void decode_flags(struct net_buf_simple *buf, struct mqtt_sn_flags *flags)
{
	uint8_t b = net_buf_simple_pull_u8(buf);

	flags->dup = (bool)(b & MQTT_SN_FLAGS_DUP);
	flags->retain = (bool)(b & MQTT_SN_FLAGS_RETAIN);
	flags->will = (bool)(b & MQTT_SN_FLAGS_WILL);
	flags->clean_session = (bool)(b & MQTT_SN_FLAGS_CLEANSESSION);

	flags->qos = (enum mqtt_sn_qos)((b & MQTT_SN_FLAGS_MASK_QOS) >> MQTT_SN_FLAGS_SHIFT_QOS);

	flags->topic_type = (enum mqtt_sn_topic_type)((b & MQTT_SN_FLAGS_MASK_TOPICID_TYPE) >>
						      MQTT_SN_FLAGS_SHIFT_TOPICID_TYPE);
}

static void decode_data(struct net_buf_simple *buf, struct mqtt_sn_data *dest)
{
	dest->size = buf->len;
	dest->data = net_buf_simple_pull_mem(buf, buf->len);
}

static int decode_empty_message(struct net_buf_simple *buf)
{
	if (buf->len) {
		LOG_ERR("Message not empty");
		return -EPROTO;
	}

	return 0;
}

static int decode_msg_advertise(struct net_buf_simple *buf, struct mqtt_sn_param_advertise *params)
{
	if (buf->len != 3) {
		return -EPROTO;
	}

	params->gw_id = net_buf_simple_pull_u8(buf);
	params->duration = net_buf_simple_pull_be16(buf);

	return 0;
}

static int decode_msg_searchgw(struct net_buf_simple *buf, struct mqtt_sn_param_searchgw *params)
{
	if (buf->len != 1) {
		return -EPROTO;
	}

	params->radius = net_buf_simple_pull_u8(buf);

	return 0;
}

static int decode_msg_gwinfo(struct net_buf_simple *buf, struct mqtt_sn_param_gwinfo *params)
{
	if (buf->len < 1) {
		return -EPROTO;
	}

	params->gw_id = net_buf_simple_pull_u8(buf);

	if (buf->len) {
		decode_data(buf, &params->gw_add);
	} else {
		params->gw_add.size = 0;
	}

	return 0;
}

static int decode_msg_connack(struct net_buf_simple *buf, struct mqtt_sn_param_connack *params)
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

static int decode_msg_register(struct net_buf_simple *buf, struct mqtt_sn_param_register *params)
{
	if (buf->len < 5) {
		return -EPROTO;
	}

	params->topic_id = net_buf_simple_pull_be16(buf);
	params->msg_id = net_buf_simple_pull_be16(buf);
	decode_data(buf, &params->topic);

	return 0;
}

static int decode_msg_regack(struct net_buf_simple *buf, struct mqtt_sn_param_regack *params)
{
	if (buf->len != 5) {
		return -EPROTO;
	}

	params->topic_id = net_buf_simple_pull_be16(buf);
	params->msg_id = net_buf_simple_pull_be16(buf);
	params->ret_code = net_buf_simple_pull_u8(buf);

	return 0;
}

static int decode_msg_publish(struct net_buf_simple *buf, struct mqtt_sn_param_publish *params)
{
	struct mqtt_sn_flags flags;

	if (buf->len < 6) {
		return -EPROTO;
	}

	decode_flags(buf, &flags);
	params->dup = flags.dup;
	params->qos = flags.qos;
	params->retain = flags.retain;
	params->topic_type = flags.topic_type;
	params->topic_id = net_buf_simple_pull_be16(buf);
	params->msg_id = net_buf_simple_pull_be16(buf);
	decode_data(buf, &params->data);

	return 0;
}

static int decode_msg_puback(struct net_buf_simple *buf, struct mqtt_sn_param_puback *params)
{
	if (buf->len != 5) {
		return -EPROTO;
	}

	params->topic_id = net_buf_simple_pull_be16(buf);
	params->msg_id = net_buf_simple_pull_be16(buf);
	params->ret_code = net_buf_simple_pull_u8(buf);

	return 0;
}

static int decode_msg_pubrec(struct net_buf_simple *buf, struct mqtt_sn_param_pubrec *params)
{
	if (buf->len != 2) {
		return -EPROTO;
	}

	params->msg_id = net_buf_simple_pull_be16(buf);

	return 0;
}

static int decode_msg_pubrel(struct net_buf_simple *buf, struct mqtt_sn_param_pubrel *params)
{
	if (buf->len != 2) {
		return -EPROTO;
	}

	params->msg_id = net_buf_simple_pull_be16(buf);

	return 0;
}

static int decode_msg_pubcomp(struct net_buf_simple *buf, struct mqtt_sn_param_pubcomp *params)
{
	if (buf->len != 2) {
		return -EPROTO;
	}

	params->msg_id = net_buf_simple_pull_be16(buf);

	return 0;
}

static int decode_msg_suback(struct net_buf_simple *buf, struct mqtt_sn_param_suback *params)
{
	struct mqtt_sn_flags flags;

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

static int decode_msg_unsuback(struct net_buf_simple *buf, struct mqtt_sn_param_unsuback *params)
{
	if (buf->len != 2) {
		return -EPROTO;
	}

	params->msg_id = net_buf_simple_pull_be16(buf);

	return 0;
}

static int decode_msg_pingreq(struct net_buf_simple *buf)
{
	/* The client_id field is only set if the message was sent by a client. */
	return decode_empty_message(buf);
}

static int decode_msg_pingresp(struct net_buf_simple *buf)
{
	return decode_empty_message(buf);
}

static int decode_msg_disconnect(struct net_buf_simple *buf,
				 struct mqtt_sn_param_disconnect *params)
{
	/* The duration field is only set if the message was sent by a client. */
	return decode_empty_message(buf);
}

static int decode_msg_willtopicresp(struct net_buf_simple *buf,
				    struct mqtt_sn_param_willtopicresp *params)
{
	if (buf->len != 1) {
		return -EPROTO;
	}

	params->ret_code = net_buf_simple_pull_u8(buf);

	return 0;
}

static int decode_msg_willmsgresp(struct net_buf_simple *buf,
				  struct mqtt_sn_param_willmsgresp *params)
{
	if (buf->len != 1) {
		return -EPROTO;
	}

	params->ret_code = net_buf_simple_pull_u8(buf);

	return 0;
}

int mqtt_sn_decode_msg(struct net_buf_simple *buf, struct mqtt_sn_param *params)
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

	params->type = (enum mqtt_sn_msg_type)net_buf_simple_pull_u8(buf);

	LOG_INF("Decoding message type: %d", params->type);

	switch (params->type) {
	case MQTT_SN_MSG_TYPE_ADVERTISE:
		return decode_msg_advertise(buf, &params->params.advertise);
	case MQTT_SN_MSG_TYPE_SEARCHGW:
		return decode_msg_searchgw(buf, &params->params.searchgw);
	case MQTT_SN_MSG_TYPE_GWINFO:
		return decode_msg_gwinfo(buf, &params->params.gwinfo);
	case MQTT_SN_MSG_TYPE_CONNACK:
		return decode_msg_connack(buf, &params->params.connack);
	case MQTT_SN_MSG_TYPE_WILLTOPICREQ:
		return decode_msg_willtopicreq(buf);
	case MQTT_SN_MSG_TYPE_WILLMSGREQ:
		return decode_msg_willmsgreq(buf);
	case MQTT_SN_MSG_TYPE_REGISTER:
		return decode_msg_register(buf, &params->params.reg);
	case MQTT_SN_MSG_TYPE_REGACK:
		return decode_msg_regack(buf, &params->params.regack);
	case MQTT_SN_MSG_TYPE_PUBLISH:
		return decode_msg_publish(buf, &params->params.publish);
	case MQTT_SN_MSG_TYPE_PUBACK:
		return decode_msg_puback(buf, &params->params.puback);
	case MQTT_SN_MSG_TYPE_PUBREC:
		return decode_msg_pubrec(buf, &params->params.pubrec);
	case MQTT_SN_MSG_TYPE_PUBREL:
		return decode_msg_pubrel(buf, &params->params.pubrel);
	case MQTT_SN_MSG_TYPE_PUBCOMP:
		return decode_msg_pubcomp(buf, &params->params.pubcomp);
	case MQTT_SN_MSG_TYPE_SUBACK:
		return decode_msg_suback(buf, &params->params.suback);
	case MQTT_SN_MSG_TYPE_UNSUBACK:
		return decode_msg_unsuback(buf, &params->params.unsuback);
	case MQTT_SN_MSG_TYPE_PINGREQ:
		return decode_msg_pingreq(buf);
	case MQTT_SN_MSG_TYPE_PINGRESP:
		return decode_msg_pingresp(buf);
	case MQTT_SN_MSG_TYPE_DISCONNECT:
		return decode_msg_disconnect(buf, &params->params.disconnect);
	case MQTT_SN_MSG_TYPE_WILLTOPICRESP:
		return decode_msg_willtopicresp(buf, &params->params.willtopicresp);
	case MQTT_SN_MSG_TYPE_WILLMSGRESP:
		return decode_msg_willmsgresp(buf, &params->params.willmsgresp);
	default:
		LOG_ERR("Got unexpected message type %d", params->type);
		return -EINVAL;
	}
}
