/*
 * Copyright (c) 2025 Netfeasa Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/** @file mqtt_offload_encoder.c
 *
 * @brief HL78XX-MQTT message encoder.
 */

#include "mqtt_offload_msg.h"

#include <zephyr/net/mqtt_offload.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(net_mqtt_offload_encoder, CONFIG_MQTT_OFFLOAD_LOG_LEVEL);

/**
 * @brief Prepare a message
 *
 * @param buf Message struct to use
 * @param sz The length of the message's payload without the length field and the type.
 * @param type Message type
 * @return 0 or negative error code
 */
static int prepare_message(struct net_buf_simple *buf, size_t sz, enum mqtt_offload_msg_type type)
{
	/* add size of length field */
	sz += (sz > 254 ? 3 : 1);

	size_t maxlen = net_buf_simple_max_len(buf);

	LOG_DBG("Preparing message of type %d with size %zu", type, sz);

	/* Size must not be larger than an uint16_t can fit */
	if (sz > UINT16_MAX) {
		LOG_ERR("Message of size %zu is too large for HL78XX-MQTT", sz);
		return -EFBIG;
	}

	if (sz > maxlen) {
		LOG_ERR("Message of size %zu does not fit in buffer of length %zu", sz, maxlen);
		return -ENOMEM;
	}

	if (sz <= 255) {
		net_buf_simple_add_u8(buf, (uint8_t)sz);
	} else {
		net_buf_simple_add_u8(buf, MQTT_OFFLOAD_LENGTH_FIELD_EXTENDED_PREFIX);
		net_buf_simple_add_be16(buf, (uint16_t)sz);
	}

	net_buf_simple_add_u8(buf, (uint8_t)type);
	LOG_HEXDUMP_DBG(buf->data, buf->len, "Encoded message");

	return 0;
}

static void encode_flags(struct net_buf_simple *buf, struct mqtt_offload_flags *flags)
{
	uint8_t b = 0;

	LOG_DBG("Encode flags %d, %d, %d, %d", flags->retain, flags->will, flags->clean_session,
		flags->qos);

	b |= flags->retain ? MQTT_OFFLOAD_FLAGS_RETAIN : 0;
	b |= flags->will ? MQTT_OFFLOAD_FLAGS_WILL : 0;
	b |= flags->clean_session ? MQTT_OFFLOAD_FLAGS_CLEANSESSION : 0;

	b |= ((flags->qos << MQTT_OFFLOAD_FLAGS_SHIFT_QOS) & MQTT_OFFLOAD_FLAGS_MASK_QOS);

	net_buf_simple_add_u8(buf, b);
}

static int mqtt_offload_encode_msg_connect(struct net_buf_simple *buf,
					   struct mqtt_offload_param_connect *params)
{
	size_t msgsz = 4 + params->client_id.size;
	struct mqtt_offload_flags flags = {.will = params->will,
					   .clean_session = params->clean_session};
	int err;

	err = prepare_message(buf, msgsz, MQTT_OFFLOAD_MSG_TYPE_CONNECT);
	if (err) {
		return err;
	}

	encode_flags(buf, &flags);

	net_buf_simple_add_u8(buf, MQTT_OFFLOAD_PROTOCOL_ID);

	net_buf_simple_add_be16(buf, params->duration);

	net_buf_simple_add_data(buf, &params->client_id);
	return 0;
}

static int mqtt_offload_encode_msg_willtopic(struct net_buf_simple *buf,
					     struct mqtt_offload_param_willtopic *params)
{
	size_t msgsz = 1 + params->topic.size;
	struct mqtt_offload_flags flags = {.qos = params->qos, .retain = params->retain};
	int err;

	err = prepare_message(buf, msgsz, MQTT_OFFLOAD_MSG_TYPE_WILLTOPIC);
	if (err) {
		return err;
	}

	encode_flags(buf, &flags);

	net_buf_simple_add_data(buf, &params->topic);

	return 0;
}

static int mqtt_offload_encode_msg_willmsg(struct net_buf_simple *buf,
					   struct mqtt_offload_param_willmsg *params)
{
	size_t msgsz = params->msg.size;
	int err;

	err = prepare_message(buf, msgsz, MQTT_OFFLOAD_MSG_TYPE_WILLMSG);
	if (err) {
		return err;
	}

	net_buf_simple_add_data(buf, &params->msg);

	return 0;
}

static int mqtt_offload_encode_msg_publish(struct net_buf_simple *buf,
					   struct mqtt_offload_param_publish *params)
{
	/* len + type + flags + topic_len + topic + payload */
	size_t msgsz = 4 + params->topic.size + params->data.size;
	struct mqtt_offload_flags flags = {.retain = params->retain, .qos = params->qos};
	int err;

	err = prepare_message(buf, msgsz, MQTT_OFFLOAD_MSG_TYPE_PUBLISH);
	if (err) {
		return err;
	}
	encode_flags(buf, &flags);

	net_buf_simple_add_be16(buf, params->topic.size);

	net_buf_simple_add_data(buf, &params->topic);

	net_buf_simple_add_data(buf, &params->data);

	return 0;
}

static int mqtt_offload_encode_msg_puback(struct net_buf_simple *buf,
					  struct mqtt_offload_param_puback *params)
{
	size_t msgsz = 3;
	int err;

	err = prepare_message(buf, msgsz, MQTT_OFFLOAD_MSG_TYPE_PUBACK);
	if (err) {
		return err;
	}

	net_buf_simple_add_be16(buf, params->msg_id);
	net_buf_simple_add_u8(buf, params->ret_code);

	return 0;
}

static int mqtt_offload_encode_msg_subscribe(struct net_buf_simple *buf,
					     struct mqtt_offload_param_subscribe *params)
{
	size_t msgsz = 2;

	struct mqtt_offload_flags flags = {.qos = params->qos};
	int err;

	msgsz += params->topic_name.size;

	err = prepare_message(buf, msgsz, MQTT_OFFLOAD_MSG_TYPE_SUBSCRIBE);
	if (err) {
		return err;
	}
	encode_flags(buf, &flags);

	net_buf_simple_add_data(buf, &params->topic_name);

	return 0;
}

static int mqtt_offload_encode_msg_unsubscribe(struct net_buf_simple *buf,
					       struct mqtt_offload_param_unsubscribe *params)
{
	size_t msgsz = 2;

	struct mqtt_offload_flags flags = {0};

	msgsz += params->topic.topic_name.size;
	int err;

	err = prepare_message(buf, msgsz, MQTT_OFFLOAD_MSG_TYPE_UNSUBSCRIBE);
	if (err) {
		return err;
	}
	encode_flags(buf, &flags);

	net_buf_simple_add_data(buf, &params->topic.topic_name);

	return 0;
}

static int mqtt_offload_encode_msg_disconnect(struct net_buf_simple *buf,
					      struct mqtt_offload_param_disconnect *params)
{
	size_t msgsz = params->duration ? 2 : 0;
	int err;

	err = prepare_message(buf, msgsz, MQTT_OFFLOAD_MSG_TYPE_DISCONNECT);
	if (err) {
		return err;
	}

	if (params->duration) {
		net_buf_simple_add_be16(buf, params->duration);
	}

	return 0;
}

static int mqtt_offload_encode_msg_willtopicupd(struct net_buf_simple *buf,
						struct mqtt_offload_param_willtopicupd *params)
{
	size_t msgsz = 0;
	struct mqtt_offload_flags flags = {.qos = params->qos, .retain = params->retain};
	int err;

	if (params->topic.size) {
		msgsz += 1 + params->topic.size;
	}

	err = prepare_message(buf, msgsz, MQTT_OFFLOAD_MSG_TYPE_WILLTOPICUPD);
	if (err) {
		return err;
	}

	/* If the topic is empty, send an empty message to delete the will topic & message. */
	if (params->topic.size) {
		encode_flags(buf, &flags);

		net_buf_simple_add_data(buf, &params->topic);
	}

	return 0;
}

static int mqtt_offload_encode_msg_willmsgupd(struct net_buf_simple *buf,
					      struct mqtt_offload_param_willmsgupd *params)
{
	size_t msgsz = params->msg.size;
	int err;

	err = prepare_message(buf, msgsz, MQTT_OFFLOAD_MSG_TYPE_WILLMSGUPD);
	if (err) {
		return err;
	}

	net_buf_simple_add_data(buf, &params->msg);

	return 0;
}

int mqtt_offload_encode_msg(struct net_buf_simple *buf, struct mqtt_offload_param *param)
{
	int result;

	if (buf->len) {
		LOG_ERR("Buffer not clean - bug?");
		return -EBUSY;
	}

	LOG_DBG("Encoding message of type %d", param->type);

	switch (param->type) {
	case MQTT_OFFLOAD_MSG_TYPE_CONNECT:
		result = mqtt_offload_encode_msg_connect(buf, &param->params.connect);
		break;
	case MQTT_OFFLOAD_MSG_TYPE_WILLTOPIC:
		result = mqtt_offload_encode_msg_willtopic(buf, &param->params.willtopic);
		break;
	case MQTT_OFFLOAD_MSG_TYPE_WILLMSG:
		result = mqtt_offload_encode_msg_willmsg(buf, &param->params.willmsg);
		break;
	case MQTT_OFFLOAD_MSG_TYPE_PUBLISH:
		result = mqtt_offload_encode_msg_publish(buf, &param->params.publish);
		break;
	case MQTT_OFFLOAD_MSG_TYPE_PUBACK:
		result = mqtt_offload_encode_msg_puback(buf, &param->params.puback);
		break;
	case MQTT_OFFLOAD_MSG_TYPE_SUBSCRIBE:
		result = mqtt_offload_encode_msg_subscribe(buf, &param->params.subscribe);
		break;
	case MQTT_OFFLOAD_MSG_TYPE_UNSUBSCRIBE:
		result = mqtt_offload_encode_msg_unsubscribe(buf, &param->params.unsubscribe);
		break;
	case MQTT_OFFLOAD_MSG_TYPE_DISCONNECT:
		result = mqtt_offload_encode_msg_disconnect(buf, &param->params.disconnect);
		break;
	case MQTT_OFFLOAD_MSG_TYPE_WILLTOPICUPD:
		result = mqtt_offload_encode_msg_willtopicupd(buf, &param->params.willtopicupd);
		break;
	case MQTT_OFFLOAD_MSG_TYPE_WILLMSGUPD:
		result = mqtt_offload_encode_msg_willmsgupd(buf, &param->params.willmsgupd);
		break;
	default:
		LOG_ERR("Unsupported msg type %d", param->type);
		result = -ENOTSUP;
		break;
	}

	return result;
}
