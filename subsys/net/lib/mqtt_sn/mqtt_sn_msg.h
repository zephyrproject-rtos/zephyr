/*
 * Copyright (c) 2022 René Beckmann
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/** @file mqtt_sn_msg.h
 *
 * @brief Function and data structures internal to MQTT-SN module.
 */

#ifndef MQTT_SN_MSG_H_
#define MQTT_SN_MSG_H_

#include <zephyr/net/mqtt_sn.h>
#include <zephyr/net_buf.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MQTT_SN_LENGTH_FIELD_EXTENDED_PREFIX 0x01
#define MQTT_SN_PROTOCOL_ID 0x01

struct mqtt_sn_flags {
	bool dup;
	enum mqtt_sn_qos qos;
	bool retain;
	bool will;
	bool clean_session;
	enum mqtt_sn_topic_type topic_type;
};

enum mqtt_sn_msg_type {
	MQTT_SN_MSG_TYPE_ADVERTISE = 0x00,
	MQTT_SN_MSG_TYPE_SEARCHGW = 0x01,
	MQTT_SN_MSG_TYPE_GWINFO = 0x02,
	MQTT_SN_MSG_TYPE_CONNECT = 0x04,
	MQTT_SN_MSG_TYPE_CONNACK = 0x05,
	MQTT_SN_MSG_TYPE_WILLTOPICREQ = 0x06,
	MQTT_SN_MSG_TYPE_WILLTOPIC = 0x07,
	MQTT_SN_MSG_TYPE_WILLMSGREQ = 0x08,
	MQTT_SN_MSG_TYPE_WILLMSG = 0x09,
	MQTT_SN_MSG_TYPE_REGISTER = 0x0A,
	MQTT_SN_MSG_TYPE_REGACK = 0x0B,
	MQTT_SN_MSG_TYPE_PUBLISH = 0x0C,
	MQTT_SN_MSG_TYPE_PUBACK = 0x0D,
	MQTT_SN_MSG_TYPE_PUBCOMP = 0x0E,
	MQTT_SN_MSG_TYPE_PUBREC = 0x0F,
	MQTT_SN_MSG_TYPE_PUBREL = 0x10,
	MQTT_SN_MSG_TYPE_SUBSCRIBE = 0x12,
	MQTT_SN_MSG_TYPE_SUBACK = 0x13,
	MQTT_SN_MSG_TYPE_UNSUBSCRIBE = 0x14,
	MQTT_SN_MSG_TYPE_UNSUBACK = 0x15,
	MQTT_SN_MSG_TYPE_PINGREQ = 0x16,
	MQTT_SN_MSG_TYPE_PINGRESP = 0x17,
	MQTT_SN_MSG_TYPE_DISCONNECT = 0x18,
	MQTT_SN_MSG_TYPE_WILLTOPICUPD = 0x1A,
	MQTT_SN_MSG_TYPE_WILLTOPICRESP = 0x1B,
	MQTT_SN_MSG_TYPE_WILLMSGUPD = 0x1C,
	MQTT_SN_MSG_TYPE_WILLMSGRESP = 0x1D,
	MQTT_SN_MSG_TYPE_ENCAPSULATED = 0xFE,
};

struct mqtt_sn_param_advertise {
	uint8_t gw_id;
	uint16_t duration;
};

struct mqtt_sn_param_searchgw {
	uint8_t radius;
};

struct mqtt_sn_param_gwinfo {
	uint8_t gw_id;
	struct mqtt_sn_data gw_add;
};

struct mqtt_sn_param_connect {
	bool will;
	bool clean_session;
	uint16_t duration;
	struct mqtt_sn_data client_id;
};

struct mqtt_sn_param_connack {
	enum mqtt_sn_return_code ret_code;
};

struct mqtt_sn_param_willtopic {
	enum mqtt_sn_qos qos;
	bool retain;
	struct mqtt_sn_data topic;
};

struct mqtt_sn_param_willmsg {
	struct mqtt_sn_data msg;
};

struct mqtt_sn_param_register {
	uint16_t msg_id;
	uint16_t topic_id;
	struct mqtt_sn_data topic;
};

struct mqtt_sn_param_regack {
	uint16_t msg_id;
	uint16_t topic_id;
	enum mqtt_sn_return_code ret_code;
};

struct mqtt_sn_param_publish {
	bool dup;
	bool retain;
	enum mqtt_sn_qos qos;
	enum mqtt_sn_topic_type topic_type;
	uint16_t topic_id;
	uint16_t msg_id;
	struct mqtt_sn_data data;
};

struct mqtt_sn_param_puback {
	uint16_t msg_id;
	uint16_t topic_id;
	enum mqtt_sn_return_code ret_code;
};

struct mqtt_sn_param_pubrec {
	uint16_t msg_id;
};

struct mqtt_sn_param_pubrel {
	uint16_t msg_id;
};

struct mqtt_sn_param_pubcomp {
	uint16_t msg_id;
};

struct mqtt_sn_param_subscribe {
	bool dup;
	enum mqtt_sn_qos qos;
	enum mqtt_sn_topic_type topic_type;
	uint16_t msg_id;
	struct {
		struct mqtt_sn_data topic_name;
		uint16_t topic_id;
	} topic;
};

struct mqtt_sn_param_suback {
	enum mqtt_sn_qos qos;
	uint16_t topic_id;
	uint16_t msg_id;
	enum mqtt_sn_return_code ret_code;
};

struct mqtt_sn_param_unsubscribe {
	enum mqtt_sn_topic_type topic_type;
	uint16_t msg_id;
	union {
		struct mqtt_sn_data topic_name;
		uint16_t topic_id;
	} topic;
};

struct mqtt_sn_param_unsuback {
	uint16_t msg_id;
};

struct mqtt_sn_param_pingreq {
	struct mqtt_sn_data client_id;
};

struct mqtt_sn_param_disconnect {
	uint16_t duration;
};

struct mqtt_sn_param_willtopicupd {
	enum mqtt_sn_qos qos;
	bool retain;
	struct mqtt_sn_data topic;
};

struct mqtt_sn_param_willmsgupd {
	struct mqtt_sn_data msg;
};

struct mqtt_sn_param_willtopicresp {
	enum mqtt_sn_return_code ret_code;
};

struct mqtt_sn_param_willmsgresp {
	enum mqtt_sn_return_code ret_code;
};

struct mqtt_sn_param {
	enum mqtt_sn_msg_type type;
	union {
		struct mqtt_sn_param_advertise advertise;
		struct mqtt_sn_param_searchgw searchgw;
		struct mqtt_sn_param_gwinfo gwinfo;
		struct mqtt_sn_param_connect connect;
		struct mqtt_sn_param_connack connack;
		struct mqtt_sn_param_willtopic willtopic;
		struct mqtt_sn_param_willmsg willmsg;
		struct mqtt_sn_param_register reg;
		struct mqtt_sn_param_regack regack;
		struct mqtt_sn_param_publish publish;
		struct mqtt_sn_param_puback puback;
		struct mqtt_sn_param_pubrec pubrec;
		struct mqtt_sn_param_pubrel pubrel;
		struct mqtt_sn_param_pubcomp pubcomp;
		struct mqtt_sn_param_subscribe subscribe;
		struct mqtt_sn_param_suback suback;
		struct mqtt_sn_param_unsubscribe unsubscribe;
		struct mqtt_sn_param_unsuback unsuback;
		struct mqtt_sn_param_pingreq pingreq;
		struct mqtt_sn_param_disconnect disconnect;
		struct mqtt_sn_param_willtopicupd willtopicupd;
		struct mqtt_sn_param_willmsgupd willmsgupd;
		struct mqtt_sn_param_willtopicresp willtopicresp;
		struct mqtt_sn_param_willmsgresp willmsgresp;
	} params;
};

/**@brief MQTT-SN Flags-field bitmasks */
#define MQTT_SN_FLAGS_DUP BIT(7)
#define MQTT_SN_FLAGS_QOS_0 0
#define MQTT_SN_FLAGS_QOS_1 BIT(5)
#define MQTT_SN_FLAGS_QOS_2 BIT(6)
#define MQTT_SN_FLAGS_QOS_M1 BIT(5) | BIT(6)
#define MQTT_SN_FLAGS_MASK_QOS (BIT(5) | BIT(6))
#define MQTT_SN_FLAGS_SHIFT_QOS 5
#define MQTT_SN_FLAGS_RETAIN BIT(4)
#define MQTT_SN_FLAGS_WILL BIT(3)
#define MQTT_SN_FLAGS_CLEANSESSION BIT(2)
#define MQTT_SN_FLAGS_TOPICID_TYPE_NORMAL 0
#define MQTT_SN_FLAGS_TOPICID_TYPE_PREDEF BIT(0)
#define MQTT_SN_FLAGS_TOPICID_TYPE_SHORT BIT(1)
#define MQTT_SN_FLAGS_MASK_TOPICID_TYPE (BIT(0) | BIT(1))
#define MQTT_SN_FLAGS_SHIFT_TOPICID_TYPE 0

static inline void *net_buf_simple_add_data(struct net_buf_simple *buf, struct mqtt_sn_data *data)
{
	return net_buf_simple_add_mem(buf, data->data, data->size);
}

int mqtt_sn_encode_msg(struct net_buf_simple *buf, struct mqtt_sn_param *params);

int mqtt_sn_decode_msg(struct net_buf_simple *buf, struct mqtt_sn_param *params);

#ifdef __cplusplus
}
#endif

#endif /* MQTT_SN_MSG_H_ */
