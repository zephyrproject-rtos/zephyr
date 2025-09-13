/*
 * Copyright (c) 2025 Netfeasa Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/** @file hl78xx_mqtt_offload_msg.h
 *
 * @brief Function and data structures internal to HL78XX-MQTT module.
 */

#ifndef MQTT_OFFLOAD_MSG_H_
#define MQTT_OFFLOAD_MSG_H_

#include <zephyr/net/mqtt_offload.h>
#include <zephyr/net_buf.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MQTT_OFFLOAD_LENGTH_FIELD_EXTENDED_PREFIX 0x01
#define MQTT_OFFLOAD_PROTOCOL_ID                  0x01

struct mqtt_offload_flags {
	enum mqtt_offload_qos qos;
	bool retain;
	bool will;
	bool clean_session;
};

enum mqtt_offload_msg_type {
	MQTT_OFFLOAD_MSG_TYPE_CONNECT = 0x04,
	MQTT_OFFLOAD_MSG_TYPE_CONNACK = 0x05,
	MQTT_OFFLOAD_MSG_TYPE_WILLTOPICREQ = 0x06,
	MQTT_OFFLOAD_MSG_TYPE_WILLTOPIC = 0x07,
	MQTT_OFFLOAD_MSG_TYPE_WILLMSGREQ = 0x08,
	MQTT_OFFLOAD_MSG_TYPE_WILLMSG = 0x09,
	MQTT_OFFLOAD_MSG_TYPE_PUBLISH = 0x0C,
	MQTT_OFFLOAD_MSG_TYPE_PUBACK = 0x0D,
	MQTT_OFFLOAD_MSG_TYPE_PUBCOMP = 0x0E,
	MQTT_OFFLOAD_MSG_TYPE_PUBREC = 0x0F,
	MQTT_OFFLOAD_MSG_TYPE_PUBREL = 0x10,
	MQTT_OFFLOAD_MSG_TYPE_SUBSCRIBE = 0x12,
	MQTT_OFFLOAD_MSG_TYPE_SUBACK = 0x13,
	MQTT_OFFLOAD_MSG_TYPE_UNSUBSCRIBE = 0x14,
	MQTT_OFFLOAD_MSG_TYPE_UNSUBACK = 0x15,
	MQTT_OFFLOAD_MSG_TYPE_DISCONNECT = 0x18,
	MQTT_OFFLOAD_MSG_TYPE_WILLTOPICUPD = 0x1A,
	MQTT_OFFLOAD_MSG_TYPE_WILLTOPICRESP = 0x1B,
	MQTT_OFFLOAD_MSG_TYPE_WILLMSGUPD = 0x1C,
	MQTT_OFFLOAD_MSG_TYPE_WILLMSGRESP = 0x1D,
	MQTT_OFFLOAD_MSG_TYPE_ENCAPSULATED = 0xFE,
};

struct mqtt_offload_param_advertise {
	uint8_t gw_id;
	uint16_t duration;
};

struct mqtt_offload_param_searchgw {
	uint8_t radius;
};

struct mqtt_offload_param_gwinfo {
	uint8_t gw_id;
	struct mqtt_offload_data gw_add;
};

struct mqtt_offload_param_connect {
	bool will;
	bool clean_session;
	struct mqtt_offload_data will_topic;
	struct mqtt_offload_data will_payload;
	int qos;
	bool will_retain;
	int duration;
	struct mqtt_offload_data client_id;
};

struct mqtt_offload_param_connack {
	enum mqtt_offload_return_code ret_code;
};

struct mqtt_offload_param_willtopic {
	enum mqtt_offload_qos qos;
	bool retain;
	struct mqtt_offload_data topic;
};

struct mqtt_offload_param_willmsg {
	struct mqtt_offload_data msg;
};

struct mqtt_offload_param_register {
	uint16_t msg_id;
	uint16_t topic_id;
	struct mqtt_offload_data topic;
};

struct mqtt_offload_param_regack {
	uint16_t msg_id;
	uint16_t topic_id;
	enum mqtt_offload_return_code ret_code;
};

struct mqtt_offload_param_publish {
	bool dup;
	bool retain;
	enum mqtt_offload_qos qos;
	struct mqtt_offload_data topic;
	uint16_t msg_id;
	struct mqtt_offload_data data;
};

struct mqtt_offload_param_puback {
	uint16_t msg_id;
	enum mqtt_offload_return_code ret_code;
};

struct mqtt_offload_param_pubrec {
	uint16_t msg_id;
};

struct mqtt_offload_param_pubrel {
	uint16_t msg_id;
};

struct mqtt_offload_param_pubcomp {
	uint16_t msg_id;
};

struct mqtt_offload_param_subscribe {
	bool dup;
	enum mqtt_offload_qos qos;
	/** Message id used to identify subscription request. */
	uint16_t msg_id;

	struct mqtt_offload_data topic_name;
};

struct mqtt_offload_param_suback {
	enum mqtt_offload_qos qos;
	uint16_t topic_id;
	uint16_t msg_id;
	enum mqtt_offload_return_code ret_code;
};

struct mqtt_offload_param_unsubscribe {
	uint16_t msg_id;
	union {
		struct mqtt_offload_data topic_name;
		uint16_t topic_id;
	} topic;
};

struct mqtt_offload_param_unsuback {
	uint16_t msg_id;
	enum mqtt_offload_return_code ret_code;
};

struct mqtt_offload_param_pingreq {
	struct mqtt_offload_data client_id;
};

struct mqtt_offload_param_disconnect {
	uint16_t duration;
};

struct mqtt_offload_param_willtopicupd {
	enum mqtt_offload_qos qos;
	bool retain;
	struct mqtt_offload_data topic;
};

struct mqtt_offload_param_willmsgupd {
	struct mqtt_offload_data msg;
};

struct mqtt_offload_param_willtopicresp {
	enum mqtt_offload_return_code ret_code;
};

struct mqtt_offload_param_willmsgresp {
	enum mqtt_offload_return_code ret_code;
};

struct mqtt_offload_param {
	enum mqtt_offload_msg_type type;
	union {
		struct mqtt_offload_param_advertise advertise;
		struct mqtt_offload_param_searchgw searchgw;
		struct mqtt_offload_param_gwinfo gwinfo;
		struct mqtt_offload_param_connect connect;
		struct mqtt_offload_param_connack connack;
		struct mqtt_offload_param_willtopic willtopic;
		struct mqtt_offload_param_willmsg willmsg;
		struct mqtt_offload_param_publish publish;
		struct mqtt_offload_param_puback puback;
		struct mqtt_offload_param_pubrec pubrec;
		struct mqtt_offload_param_pubrel pubrel;
		struct mqtt_offload_param_pubcomp pubcomp;
		struct mqtt_offload_param_subscribe subscribe;
		struct mqtt_offload_param_suback suback;
		struct mqtt_offload_param_unsubscribe unsubscribe;
		struct mqtt_offload_param_unsuback unsuback;
		struct mqtt_offload_param_disconnect disconnect;
		struct mqtt_offload_param_willtopicupd willtopicupd;
		struct mqtt_offload_param_willmsgupd willmsgupd;
		struct mqtt_offload_param_willtopicresp willtopicresp;
		struct mqtt_offload_param_willmsgresp willmsgresp;
	} params;
};

/**@brief MQTT Offload Flags-field bitmasks */
#define MQTT_OFFLOAD_FLAGS_DUP                 BIT(7)
#define MQTT_OFFLOAD_FLAGS_QOS_0               0
#define MQTT_OFFLOAD_FLAGS_QOS_1               BIT(5)
#define MQTT_OFFLOAD_FLAGS_QOS_2               BIT(6)
#define MQTT_OFFLOAD_FLAGS_QOS_M1              BIT(5) | BIT(6)
#define MQTT_OFFLOAD_FLAGS_MASK_QOS            (BIT(5) | BIT(6))
#define MQTT_OFFLOAD_FLAGS_SHIFT_QOS           5
#define MQTT_OFFLOAD_FLAGS_RETAIN              BIT(4)
#define MQTT_OFFLOAD_FLAGS_WILL                BIT(3)
#define MQTT_OFFLOAD_FLAGS_CLEANSESSION        BIT(2)
#define MQTT_OFFLOAD_FLAGS_TOPICID_TYPE_NORMAL 0
#define MQTT_OFFLOAD_FLAGS_TOPICID_TYPE_PREDEF BIT(0)
#define MQTT_OFFLOAD_FLAGS_TOPICID_TYPE_SHORT  BIT(1)
#define MQTT_OFFLOAD_FLAGS_MASK_TOPICID_TYPE   (BIT(0) | BIT(1))
#define MQTT_OFFLOAD_FLAGS_SHIFT_TOPICID_TYPE  0

static inline void *net_buf_simple_add_data(struct net_buf_simple *buf,
					    struct mqtt_offload_data *data)
{
	return net_buf_simple_add_mem(buf, data->data, data->size);
}

int mqtt_offload_encode_msg(struct net_buf_simple *buf, struct mqtt_offload_param *params);

int mqtt_offload_decode_msg(struct net_buf_simple *buf, struct mqtt_offload_param *params);

#ifdef __cplusplus
}
#endif

#endif /* MQTT_OFFLOAD_MSG_H_ */
