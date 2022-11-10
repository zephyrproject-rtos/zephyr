/*
 * Copyright (c) 2022 René Beckmann
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/** @file mqtt_sn_encoder.c
 *
 * @brief MQTT-SN message encoder.
 */

#include "mqtt_sn_msg.h"

#include <zephyr/net/mqtt_sn.h>

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(net_mqtt_sn, CONFIG_MQTT_SN_LOG_LEVEL);

/**
 * @brief Prepare a message
 *
 * @param buf Message struct to use
 * @param sz The length of the message's payload without the length field and the type.
 * @param type Message type
 * @return 0 or negative error code
 */
static int prepare_message(struct net_buf_simple *buf, size_t sz, enum mqtt_sn_msg_type type)
{
	/* Add one for the type field */
	sz++;
	/* add size of length field */
	sz += (sz > 254 ? 3 : 1);

	size_t maxlen = net_buf_simple_max_len(buf);

	LOG_DBG("Preparing message of type %d with size %zu", type, sz);

	/* Size must not be larger than an uint16_t can fit */
	if (sz > UINT16_MAX) {
		LOG_ERR("Message of size %zu is too large for MQTT-SN", sz);
		return -EFBIG;
	}

	if (sz > maxlen) {
		LOG_ERR("Message of size %zu does not fit in buffer of length %zu", sz, maxlen);
		return -ENOMEM;
	}

	if (sz <= 255) {
		net_buf_simple_add_u8(buf, (uint8_t)sz);
	} else {
		net_buf_simple_add_u8(buf, MQTT_SN_LENGTH_FIELD_EXTENDED_PREFIX);
		net_buf_simple_add_be16(buf, (uint16_t)sz);
	}

	net_buf_simple_add_u8(buf, (uint8_t)type);

	return 0;
}

static void encode_flags(struct net_buf_simple *buf, struct mqtt_sn_flags *flags)
{
	uint8_t b = 0;

	LOG_DBG("Encode flags %d, %d, %d, %d, %d, %d", flags->dup, flags->retain, flags->will,
		flags->clean_session, flags->qos, flags->topic_type);

	b |= flags->dup ? MQTT_SN_FLAGS_DUP : 0;
	b |= flags->retain ? MQTT_SN_FLAGS_RETAIN : 0;
	b |= flags->will ? MQTT_SN_FLAGS_WILL : 0;
	b |= flags->clean_session ? MQTT_SN_FLAGS_CLEANSESSION : 0;

	b |= ((flags->qos << MQTT_SN_FLAGS_SHIFT_QOS) & MQTT_SN_FLAGS_MASK_QOS);
	b |= ((flags->topic_type << MQTT_SN_FLAGS_SHIFT_TOPICID_TYPE) &
	      MQTT_SN_FLAGS_MASK_TOPICID_TYPE);

	net_buf_simple_add_u8(buf, b);
}

static int mqtt_sn_encode_msg_searchgw(struct net_buf_simple *buf,
				       struct mqtt_sn_param_searchgw *params)
{
	size_t msgsz = 1;
	int err;

	err = prepare_message(buf, msgsz, MQTT_SN_MSG_TYPE_SEARCHGW);
	if (err) {
		return err;
	}

	net_buf_simple_add_u8(buf, params->radius);

	return 0;
}

static int mqtt_sn_encode_msg_gwinfo(struct net_buf_simple *buf,
				     struct mqtt_sn_param_gwinfo *params)
{
	size_t msgsz = 1 + params->gw_add.size;
	int err;

	err = prepare_message(buf, msgsz, MQTT_SN_MSG_TYPE_GWINFO);
	if (err) {
		return err;
	}

	net_buf_simple_add_u8(buf, params->gw_id);
	net_buf_simple_add_data(buf, &params->gw_add);

	return 0;
}

static int mqtt_sn_encode_msg_connect(struct net_buf_simple *buf,
				      struct mqtt_sn_param_connect *params)
{
	size_t msgsz = 4 + params->client_id.size;
	struct mqtt_sn_flags flags = {.will = params->will, .clean_session = params->clean_session};
	int err;

	err = prepare_message(buf, msgsz, MQTT_SN_MSG_TYPE_CONNECT);
	if (err) {
		return err;
	}

	encode_flags(buf, &flags);

	net_buf_simple_add_u8(buf, MQTT_SN_PROTOCOL_ID);

	net_buf_simple_add_be16(buf, params->duration);

	net_buf_simple_add_data(buf, &params->client_id);

	return 0;
}

static int mqtt_sn_encode_msg_willtopic(struct net_buf_simple *buf,
					struct mqtt_sn_param_willtopic *params)
{
	size_t msgsz = 1 + params->topic.size;
	struct mqtt_sn_flags flags = {.qos = params->qos, .retain = params->retain};
	int err;

	err = prepare_message(buf, msgsz, MQTT_SN_MSG_TYPE_WILLTOPIC);
	if (err) {
		return err;
	}

	encode_flags(buf, &flags);

	net_buf_simple_add_data(buf, &params->topic);

	return 0;
}

static int mqtt_sn_encode_msg_willmsg(struct net_buf_simple *buf,
				      struct mqtt_sn_param_willmsg *params)
{
	size_t msgsz = params->msg.size;
	int err;

	err = prepare_message(buf, msgsz, MQTT_SN_MSG_TYPE_WILLMSG);
	if (err) {
		return err;
	}

	net_buf_simple_add_data(buf, &params->msg);

	return 0;
}

static int mqtt_sn_encode_msg_register(struct net_buf_simple *buf,
				       struct mqtt_sn_param_register *params)
{
	size_t msgsz = 4 + params->topic.size;
	int err;

	err = prepare_message(buf, msgsz, MQTT_SN_MSG_TYPE_REGISTER);
	if (err) {
		return err;
	}

	/* When sent by the client, the topic ID is always 0x0000 */
	net_buf_simple_add_be16(buf, 0x00);
	net_buf_simple_add_be16(buf, params->msg_id);
	net_buf_simple_add_data(buf, &params->topic);

	return 0;
}

static int mqtt_sn_encode_msg_regack(struct net_buf_simple *buf,
				     struct mqtt_sn_param_regack *params)
{
	size_t msgsz = 5;
	int err;

	err = prepare_message(buf, msgsz, MQTT_SN_MSG_TYPE_REGACK);
	if (err) {
		return err;
	}

	net_buf_simple_add_be16(buf, params->topic_id);
	net_buf_simple_add_be16(buf, params->msg_id);
	net_buf_simple_add_u8(buf, params->ret_code);

	return 0;
}

static int mqtt_sn_encode_msg_publish(struct net_buf_simple *buf,
				      struct mqtt_sn_param_publish *params)
{
	size_t msgsz = 5 + params->data.size;
	struct mqtt_sn_flags flags = {.dup = params->dup,
				      .retain = params->retain,
				      .qos = params->qos,
				      .topic_type = params->topic_type};
	int err;

	err = prepare_message(buf, msgsz, MQTT_SN_MSG_TYPE_PUBLISH);
	if (err) {
		return err;
	}
	encode_flags(buf, &flags);

	net_buf_simple_add_be16(buf, params->topic_id);

	if (params->qos == MQTT_SN_QOS_1 || params->qos == MQTT_SN_QOS_2) {
		net_buf_simple_add_be16(buf, params->msg_id);
	} else {
		/* Only relevant in case of QoS levels 1 and 2, otherwise coded 0x0000. */
		net_buf_simple_add_be16(buf, 0x0000);
	}

	net_buf_simple_add_data(buf, &params->data);

	return 0;
}

static int mqtt_sn_encode_msg_puback(struct net_buf_simple *buf,
				     struct mqtt_sn_param_puback *params)
{
	size_t msgsz = 5;
	int err;

	err = prepare_message(buf, msgsz, MQTT_SN_MSG_TYPE_PUBACK);
	if (err) {
		return err;
	}

	net_buf_simple_add_be16(buf, params->topic_id);
	net_buf_simple_add_be16(buf, params->msg_id);
	net_buf_simple_add_u8(buf, params->ret_code);

	return 0;
}

static int mqtt_sn_encode_msg_pubrec(struct net_buf_simple *buf,
				     struct mqtt_sn_param_pubrec *params)
{
	size_t msgsz = 2;
	int err;

	err = prepare_message(buf, msgsz, MQTT_SN_MSG_TYPE_PUBREC);
	if (err) {
		return err;
	}

	net_buf_simple_add_be16(buf, params->msg_id);

	return 0;
}

static int mqtt_sn_encode_msg_pubrel(struct net_buf_simple *buf,
				     struct mqtt_sn_param_pubrel *params)
{
	size_t msgsz = 2;
	int err;

	err = prepare_message(buf, msgsz, MQTT_SN_MSG_TYPE_PUBREL);
	if (err) {
		return err;
	}

	net_buf_simple_add_be16(buf, params->msg_id);

	return 0;
}

static int mqtt_sn_encode_msg_pubcomp(struct net_buf_simple *buf,
				      struct mqtt_sn_param_pubcomp *params)
{
	size_t msgsz = 2;
	int err;

	err = prepare_message(buf, msgsz, MQTT_SN_MSG_TYPE_PUBCOMP);
	if (err) {
		return err;
	}

	net_buf_simple_add_be16(buf, params->msg_id);

	return 0;
}

static int mqtt_sn_encode_msg_subscribe(struct net_buf_simple *buf,
					struct mqtt_sn_param_subscribe *params)
{
	size_t msgsz = 3;

	struct mqtt_sn_flags flags = {
		.dup = params->dup, .qos = params->qos, .topic_type = params->topic_type};
	int err;

	if (params->topic_type == MQTT_SN_TOPIC_TYPE_NORMAL) {
		msgsz += params->topic.topic_name.size;
	} else {
		msgsz += 2;
	}

	err = prepare_message(buf, msgsz, MQTT_SN_MSG_TYPE_SUBSCRIBE);
	if (err) {
		return err;
	}
	encode_flags(buf, &flags);

	net_buf_simple_add_be16(buf, params->msg_id);

	if (params->topic_type == MQTT_SN_TOPIC_TYPE_NORMAL) {
		net_buf_simple_add_data(buf, &params->topic.topic_name);
	} else {
		net_buf_simple_add_be16(buf, params->topic.topic_id);
	}

	return 0;
}

static int mqtt_sn_encode_msg_unsubscribe(struct net_buf_simple *buf,
					  struct mqtt_sn_param_unsubscribe *params)
{
	size_t msgsz = 3;

	struct mqtt_sn_flags flags = {.topic_type = params->topic_type};

	if (params->topic_type == MQTT_SN_TOPIC_TYPE_NORMAL) {
		msgsz += params->topic.topic_name.size;
	} else {
		msgsz += 2;
	}
	int err;

	err = prepare_message(buf, msgsz, MQTT_SN_MSG_TYPE_UNSUBSCRIBE);
	if (err) {
		return err;
	}
	encode_flags(buf, &flags);

	net_buf_simple_add_be16(buf, params->msg_id);

	if (params->topic_type == MQTT_SN_TOPIC_TYPE_NORMAL) {
		net_buf_simple_add_data(buf, &params->topic.topic_name);
	} else {
		net_buf_simple_add_be16(buf, params->topic.topic_id);
	}

	return 0;
}

static int mqtt_sn_encode_msg_pingreq(struct net_buf_simple *buf,
				      struct mqtt_sn_param_pingreq *params)
{
	size_t msgsz = params->client_id.size;
	int err;

	err = prepare_message(buf, msgsz, MQTT_SN_MSG_TYPE_PINGREQ);
	if (err) {
		return err;
	}

	if (params->client_id.size) {
		net_buf_simple_add_data(buf, &params->client_id);
	}

	return 0;
}

static int mqtt_sn_encode_msg_pingresp(struct net_buf_simple *buf)
{
	return prepare_message(buf, 0, MQTT_SN_MSG_TYPE_PINGRESP);
}

static int mqtt_sn_encode_msg_disconnect(struct net_buf_simple *buf,
					 struct mqtt_sn_param_disconnect *params)
{
	size_t msgsz = params->duration ? 2 : 0;
	int err;

	err = prepare_message(buf, msgsz, MQTT_SN_MSG_TYPE_DISCONNECT);
	if (err) {
		return err;
	}

	if (params->duration) {
		net_buf_simple_add_be16(buf, params->duration);
	}

	return 0;
}

static int mqtt_sn_encode_msg_willtopicupd(struct net_buf_simple *buf,
					   struct mqtt_sn_param_willtopicupd *params)
{
	size_t msgsz = 0;
	struct mqtt_sn_flags flags = {.qos = params->qos, .retain = params->retain};
	int err;

	if (params->topic.size) {
		msgsz += 1 + params->topic.size;
	}

	err = prepare_message(buf, msgsz, MQTT_SN_MSG_TYPE_WILLTOPICUPD);
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

static int mqtt_sn_encode_msg_willmsgupd(struct net_buf_simple *buf,
					 struct mqtt_sn_param_willmsgupd *params)
{
	size_t msgsz = params->msg.size;
	int err;

	err = prepare_message(buf, msgsz, MQTT_SN_MSG_TYPE_WILLMSGUPD);
	if (err) {
		return err;
	}

	net_buf_simple_add_data(buf, &params->msg);

	return 0;
}

int mqtt_sn_encode_msg(struct net_buf_simple *buf, struct mqtt_sn_param *param)
{
	int result;

	if (buf->len) {
		LOG_ERR("Buffer not clean - bug?");
		return -EBUSY;
	}

	LOG_DBG("Encoding message of type %d", param->type);

	switch (param->type) {
	case MQTT_SN_MSG_TYPE_SEARCHGW:
		result = mqtt_sn_encode_msg_searchgw(buf, &param->params.searchgw);
		break;
	case MQTT_SN_MSG_TYPE_GWINFO:
		result = mqtt_sn_encode_msg_gwinfo(buf, &param->params.gwinfo);
		break;
	case MQTT_SN_MSG_TYPE_CONNECT:
		result = mqtt_sn_encode_msg_connect(buf, &param->params.connect);
		break;
	case MQTT_SN_MSG_TYPE_WILLTOPIC:
		result = mqtt_sn_encode_msg_willtopic(buf, &param->params.willtopic);
		break;
	case MQTT_SN_MSG_TYPE_WILLMSG:
		result = mqtt_sn_encode_msg_willmsg(buf, &param->params.willmsg);
		break;
	case MQTT_SN_MSG_TYPE_REGISTER:
		result = mqtt_sn_encode_msg_register(buf, &param->params.reg);
		break;
	case MQTT_SN_MSG_TYPE_REGACK:
		result = mqtt_sn_encode_msg_regack(buf, &param->params.regack);
		break;
	case MQTT_SN_MSG_TYPE_PUBLISH:
		result = mqtt_sn_encode_msg_publish(buf, &param->params.publish);
		break;
	case MQTT_SN_MSG_TYPE_PUBACK:
		result = mqtt_sn_encode_msg_puback(buf, &param->params.puback);
		break;
	case MQTT_SN_MSG_TYPE_PUBREC:
		result = mqtt_sn_encode_msg_pubrec(buf, &param->params.pubrec);
		break;
	case MQTT_SN_MSG_TYPE_PUBREL:
		result = mqtt_sn_encode_msg_pubrel(buf, &param->params.pubrel);
		break;
	case MQTT_SN_MSG_TYPE_PUBCOMP:
		result = mqtt_sn_encode_msg_pubcomp(buf, &param->params.pubcomp);
		break;
	case MQTT_SN_MSG_TYPE_SUBSCRIBE:
		result = mqtt_sn_encode_msg_subscribe(buf, &param->params.subscribe);
		break;
	case MQTT_SN_MSG_TYPE_UNSUBSCRIBE:
		result = mqtt_sn_encode_msg_unsubscribe(buf, &param->params.unsubscribe);
		break;
	case MQTT_SN_MSG_TYPE_PINGREQ:
		result = mqtt_sn_encode_msg_pingreq(buf, &param->params.pingreq);
		break;
	case MQTT_SN_MSG_TYPE_PINGRESP:
		result = mqtt_sn_encode_msg_pingresp(buf);
		break;
	case MQTT_SN_MSG_TYPE_DISCONNECT:
		result = mqtt_sn_encode_msg_disconnect(buf, &param->params.disconnect);
		break;
	case MQTT_SN_MSG_TYPE_WILLTOPICUPD:
		result = mqtt_sn_encode_msg_willtopicupd(buf, &param->params.willtopicupd);
		break;
	case MQTT_SN_MSG_TYPE_WILLMSGUPD:
		result = mqtt_sn_encode_msg_willmsgupd(buf, &param->params.willmsgupd);
		break;
	default:
		LOG_ERR("Unsupported msg type %d", param->type);
		result = -ENOTSUP;
		break;
	}

	return result;
}
