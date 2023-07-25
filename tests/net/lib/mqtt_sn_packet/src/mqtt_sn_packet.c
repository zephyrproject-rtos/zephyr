/*
 * Copyright (c) 2022 René Beckmann
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/tc_util.h>
#include <mqtt_sn_msg.h>
#include <zephyr/net/mqtt_sn.h>
#include <zephyr/sys/util.h> /* for ARRAY_SIZE */
#include <zephyr/ztest.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(test);

#define CLIENTID MQTT_UTF8_LITERAL("zephyr")
#define BUFFER_SIZE 128

struct mqtt_sn_decode_test {
	uint8_t *data;
	size_t datasz;
	char *name;
	struct mqtt_sn_param expected;
};

#define MQTT_SN_DECODE_TEST(dataset)                                                               \
	.data = dataset, .datasz = sizeof(dataset), .name = STRINGIFY(dataset)

/* advertise gw id 42, duration 0xDEAD */
static uint8_t advertise[] = { 5, 0x00, 42, 0xDE, 0xAD };
/* searchgw with radius 1 */
static uint8_t searchgw[] = { 3, 0x01, 1 };
/* gwinfo gw id 42, address 127.0.0.1 */
static uint8_t gwinfo[] = { 7, 0x02, 42, 127, 0, 0, 1 };
/* connect with flags [will, clean_session], duration 300, client id "zephyrclient" */
static uint8_t connect[] = { 18,  0x04, 0x0C, 0x01, 1,	 44,  'z', 'e', 'p',
			     'h', 'y',	'r',  'c',  'l', 'i', 'e', 'n', 't' };
/* connack with return code accepted */
static uint8_t connack1[] = { 3, 0x05, 0x00 };
/* connack with extended length field and return code rejected - invalid topic id */
static uint8_t connack2[] = { 0x01, 0, 5, 0x05, 0x02 };
/* empty message */
static uint8_t willtopicreq[] = { 2, 0x06 };
/* willtopic with flags [qos 1, retain], topic "/zephyr" */
static uint8_t willtopic[] = { 10, 0x07, 0x30, '/', 'z', 'e', 'p', 'h', 'y', 'r' };
/* empty message */
static uint8_t willmsgreq[] = { 2, 0x08 };
/* willmsg with msg "mywill" */
static uint8_t willmsg[] = { 8, 0x09, 'm', 'y', 'w', 'i', 'l', 'l' };
/* registration with topic ID 0x1A1B, msg ID 0x1C1D, topic "/zephyr" */
static uint8_t reg[] = { 13, 0x0A, 0x1A, 0x1B, 0x1C, 0x1D, '/', 'z', 'e', 'p', 'h', 'y', 'r' };
/* registration with topic ID 0x0000, msg ID 0x1C1D, topic "/zephyr" */
static uint8_t reg_client[] = {
	13, 0x0A, 0x00, 0x00, 0x1C, 0x1D, '/', 'z', 'e', 'p', 'h', 'y', 'r'
};
/* registration ack with topic ID 0x1A1B, msg ID 0x1C1D, return code accepted */
static uint8_t regack[] = { 7, 0x0B, 0x1A, 0x1B, 0x1C, 0x1D, 0 };
/* publish message with flags [DUP, QOS2, Retain, Short Topic Type], topic ID "RB", msg ID 0x1C1D,
 * data "zephyr"
 */
static uint8_t publish[] = { 13, 0x0C, 0xD2, 'R', 'B', 0x1C, 0x1D, 'z', 'e', 'p', 'h', 'y', 'r' };
/* publish ack with topic ID 0x1A1B, msg ID 0x1C1D, return code rejected: not supported */
static uint8_t puback[] = { 7, 0x0D, 0x1A, 0x1B, 0x1C, 0x1D, 0x03 };
/* pubrec */
static uint8_t pubrec[] = { 4, 0x0F, 0xBE, 0xEF };
/* pubrel */
static uint8_t pubrel[] = { 4, 0x10, 0xBE, 0xEF };
/* pubcomp */
static uint8_t pubcomp[] = { 4, 0x0E, 0xBE, 0xEF };
/* subscribe with flags [DUP, QOS0, topic name], message ID 0x1C1D, for topic "/zephyr" */
static uint8_t subscribe[] = { 12, 0x12, 0x80, 0x1C, 0x1D, '/', 'z', 'e', 'p', 'h', 'y', 'r' };
/* subscribe ack with flags [QOS-1], topic ID 0x1A1B, msg ID 0x1909,
 * return code rejected - congested
 */
static uint8_t suback[] = { 8, 0x13, 0x60, 0x1A, 0x1B, 0x19, 0x09, 0x01 };
/* unsubscribe with flags [predefined topic ID], message ID 0x1C1D, for topic 0x1234 */
static uint8_t unsubscribe[] = { 7, 0x14, 0x01, 0x1C, 0x1D, 0x12, 0x34 };
/* unsubscribe ack msg ID 0x1337 */
static uint8_t unsuback[] = { 4, 0x15, 0x13, 0x37 };
/* pingreq from client "zephyrclient" */
static uint8_t pingreq[] = { 14, 0x16, 'z', 'e', 'p', 'h', 'y', 'r', 'c', 'l', 'i', 'e', 'n', 't' };
/* pingreq - empty */
static uint8_t pingreq1[] = { 2, 0x16 };
/* pingresp */
static uint8_t pingresp[] = { 2, 0x17 };
/* disconnect by client with duration 10000 */
static uint8_t disconnect[] = { 4, 0x18, 0x27, 0x10 };
/* empty disconnect by GW */
static uint8_t disconnect_gw[] = { 2, 0x18 };
/* willtopicupd with flags [QOS0, retain], topic "/zephyr" */
static uint8_t willtopicupd[] = { 10, 0x1A, 0x10, '/', 'z', 'e', 'p', 'h', 'y', 'r' };
/* willmsgupd with message "mywill" */
static uint8_t willmsgupd[] = { 8, 0x1C, 'm', 'y', 'w', 'i', 'l', 'l' };
/* willtopicresp */
static uint8_t willtopicresp[] = { 3, 0x1B, 0 };
/* willmsgresp */
static uint8_t willmsgresp[] = { 3, 0x1D, 0 };

static struct mqtt_sn_decode_test decode_tests[] = {
	{
		MQTT_SN_DECODE_TEST(advertise),
		.expected = (struct mqtt_sn_param){
			.type = MQTT_SN_MSG_TYPE_ADVERTISE,
			.params.advertise = {
				.gw_id = 42,
				.duration = 0xDEAD
			}
		}
	},
	{
		MQTT_SN_DECODE_TEST(gwinfo),
		.expected = (struct mqtt_sn_param){
			.type = MQTT_SN_MSG_TYPE_GWINFO,
			.params.gwinfo = {
				.gw_id = 42,
				.gw_add = {
					.data = gwinfo+3,
					.size = 4
				}
			}
		}
	},
	{
		MQTT_SN_DECODE_TEST(connack1),
		.expected = (struct mqtt_sn_param){
			.type = MQTT_SN_MSG_TYPE_CONNACK,
			.params.connack = {
				.ret_code = MQTT_SN_CODE_ACCEPTED
			}
		}
	},
	{
		MQTT_SN_DECODE_TEST(connack2),
		.expected = (struct mqtt_sn_param){
			.type = MQTT_SN_MSG_TYPE_CONNACK,
			.params.connack = {
				.ret_code = MQTT_SN_CODE_REJECTED_TOPIC_ID
			}
		}
	},
	{
		MQTT_SN_DECODE_TEST(willtopicreq),
		.expected = (struct mqtt_sn_param){
			.type = MQTT_SN_MSG_TYPE_WILLTOPICREQ
		}
	},
	{
		MQTT_SN_DECODE_TEST(willmsgreq),
		.expected = (struct mqtt_sn_param){
			.type = MQTT_SN_MSG_TYPE_WILLMSGREQ
		}
	},
	{
		MQTT_SN_DECODE_TEST(reg),
		.expected = (struct mqtt_sn_param){
			.type = MQTT_SN_MSG_TYPE_REGISTER,
			.params.reg = {
				.topic_id = 0x1A1B,
				.msg_id = 0x1C1D,
				.topic = {
					.data = reg + 6,
					.size = 7
				}
			}
		}
	},
	{
		MQTT_SN_DECODE_TEST(regack),
		.expected = (struct mqtt_sn_param){
			.type = MQTT_SN_MSG_TYPE_REGACK,
			.params.regack = {
				.topic_id = 0x1A1B,
				.msg_id = 0x1C1D,
				.ret_code = MQTT_SN_CODE_ACCEPTED
			}
		}
	},
	{
		MQTT_SN_DECODE_TEST(publish),
		.expected = (struct mqtt_sn_param){
			.type = MQTT_SN_MSG_TYPE_PUBLISH,
			.params.publish = {
				.dup = true,
				.qos = MQTT_SN_QOS_2,
				.retain = true,
				.topic_type = MQTT_SN_TOPIC_TYPE_SHORT,
				.topic_id = 0x5242, /* "RB" in hex */
				.msg_id = 0x1C1D,
				.data = {
					.data = publish +  7,
					.size = 6
				}
			}
		}
	},
	{
		MQTT_SN_DECODE_TEST(puback),
		.expected = (struct mqtt_sn_param){
			.type = MQTT_SN_MSG_TYPE_PUBACK,
			.params.puback = {
				.topic_id = 0x1A1B,
				.msg_id = 0x1C1D,
				.ret_code = MQTT_SN_CODE_REJECTED_NOTSUP
			}
		}
	},
	{
		MQTT_SN_DECODE_TEST(pubrec),
		.expected = (struct mqtt_sn_param){
			.type = MQTT_SN_MSG_TYPE_PUBREC,
			.params.puback = {
				.msg_id = 0xBEEF,
			}
		}
	},
	{
		MQTT_SN_DECODE_TEST(pubrel),
		.expected = (struct mqtt_sn_param){
			.type = MQTT_SN_MSG_TYPE_PUBREL,
			.params.pubrel = {
				.msg_id = 0xBEEF,
			}
		}
	},
	{
		MQTT_SN_DECODE_TEST(pubcomp),
		.expected = (struct mqtt_sn_param){
			.type = MQTT_SN_MSG_TYPE_PUBCOMP,
			.params.pubcomp = {
				.msg_id = 0xBEEF,
			}
		}
	},
	{
		MQTT_SN_DECODE_TEST(suback),
		.expected = (struct mqtt_sn_param){
			.type = MQTT_SN_MSG_TYPE_SUBACK,
			.params.suback = {
				.topic_id = 0x1A1B,
				.msg_id = 0x1909,
				.qos = MQTT_SN_QOS_M1,
				.ret_code = MQTT_SN_CODE_REJECTED_CONGESTION
			}
		}
	},
	{
		MQTT_SN_DECODE_TEST(unsuback),
		.expected = (struct mqtt_sn_param){
			.type = MQTT_SN_MSG_TYPE_UNSUBACK,
			.params.unsuback = {
				.msg_id = 0x1337,
			}
		}
	},
	{
		MQTT_SN_DECODE_TEST(pingreq1),
		.expected = (struct mqtt_sn_param){
			.type = MQTT_SN_MSG_TYPE_PINGREQ,
		}
	},
	{
		MQTT_SN_DECODE_TEST(pingresp),
		.expected = (struct mqtt_sn_param){
			.type = MQTT_SN_MSG_TYPE_PINGRESP,
		}
	},
	{
		MQTT_SN_DECODE_TEST(disconnect_gw),
		.expected = (struct mqtt_sn_param){
			.type = MQTT_SN_MSG_TYPE_DISCONNECT,
		}
	},
	{
		MQTT_SN_DECODE_TEST(willtopicresp),
		.expected = (struct mqtt_sn_param){
			.type = MQTT_SN_MSG_TYPE_WILLTOPICRESP,
		}
	},
	{
		MQTT_SN_DECODE_TEST(willmsgresp),
		.expected = (struct mqtt_sn_param){
			.type = MQTT_SN_MSG_TYPE_WILLMSGRESP,
		}
	},
};

static ZTEST(mqtt_sn_packet, test_mqtt_packet_decode)
{
	struct net_buf_simple msg;
	struct mqtt_sn_param param;
	int err;

	for (size_t i = 0; i < ARRAY_SIZE(decode_tests); i++) {
		TC_PRINT("%s - test %zu: %s\n", __func__, i, decode_tests[i].name);
		LOG_HEXDUMP_DBG(decode_tests[i].data, decode_tests[i].datasz, "Test data");
		if (i == 4) {
			LOG_HEXDUMP_INF(willtopicreq, sizeof(willtopicreq), "willtopicreq");
		}
		memset(&param, 0, sizeof(param));
		net_buf_simple_init_with_data(&msg, decode_tests[i].data, decode_tests[i].datasz);

		err = mqtt_sn_decode_msg(&msg, &param);
		zassert_equal(err, 0, "Unexpected error %d", err);

		LOG_HEXDUMP_DBG(&param, sizeof(param), "Decoded data");
		LOG_HEXDUMP_DBG(&decode_tests[i].expected, sizeof(param), "Expected data");
		zassert_mem_equal(&param, &decode_tests[i].expected, sizeof(param), "in test %zu",
				  i);
	}
}

struct mqtt_sn_encode_test {
	const char *name;
	const uint8_t *expected;
	size_t expectedsz;
	struct mqtt_sn_param p;
};

#define MQTT_SN_ENCODE_TEST(_name)                                                                 \
	.name = STRINGIFY(_name), .expected = _name, .expectedsz = sizeof(_name)

static struct mqtt_sn_encode_test encode_tests[] = {
	{
		MQTT_SN_ENCODE_TEST(searchgw),
		.p = {
			.type = MQTT_SN_MSG_TYPE_SEARCHGW,
			.params.searchgw = {
				.radius = 1
			}
		}
	},
	{
		MQTT_SN_ENCODE_TEST(gwinfo),
		.p = {
			.type = MQTT_SN_MSG_TYPE_GWINFO,
			.params.gwinfo = {
				.gw_id = 42,
				.gw_add = MQTT_SN_DATA_BYTES(127, 0, 0, 1)
			}
		}
	},
	{
		MQTT_SN_ENCODE_TEST(connect),
		.p = {
			.type = MQTT_SN_MSG_TYPE_CONNECT,
			.params.connect = {
				.will = true,
				.clean_session = true,
				.client_id = MQTT_SN_DATA_STRING_LITERAL("zephyrclient"),
				.duration = 300,
			}
		}
	},
	{
		MQTT_SN_ENCODE_TEST(willtopic),
		.p = {
			.type = MQTT_SN_MSG_TYPE_WILLTOPIC,
			.params.willtopic = {
				.qos = MQTT_SN_QOS_1,
				.retain = true,
				.topic = MQTT_SN_DATA_STRING_LITERAL("/zephyr")
			}
		}
	},
	{
		MQTT_SN_ENCODE_TEST(willmsg),
		.p = {
			.type = MQTT_SN_MSG_TYPE_WILLMSG,
			.params.willmsg = {
				.msg = MQTT_SN_DATA_STRING_LITERAL("mywill")
			}
		}
	},
	{
		MQTT_SN_ENCODE_TEST(reg_client),
		.p = {
			.type = MQTT_SN_MSG_TYPE_REGISTER,
			.params.reg = {
				/*
				 * The client must not encode the topic ID -
				 * check this is followed
				 */
				.topic_id = 0x1A1B,
				.msg_id = 0x1C1D,
				.topic = MQTT_SN_DATA_STRING_LITERAL("/zephyr")
			}
		}
	},
	{
		MQTT_SN_ENCODE_TEST(regack),
		.p = {
			.type = MQTT_SN_MSG_TYPE_REGACK,
			.params.regack = {
				.topic_id = 0x1A1B,
				.msg_id = 0x1C1D,
				.ret_code = MQTT_SN_CODE_ACCEPTED
			}
		}
	},
	{
		MQTT_SN_ENCODE_TEST(publish),
		.p = {
			.type = MQTT_SN_MSG_TYPE_PUBLISH,
			.params.publish = {
				.dup = true,
				.qos = MQTT_SN_QOS_2,
				.retain = true,

				.topic_id = 0x5242,
				.topic_type = MQTT_SN_TOPIC_TYPE_SHORT,
				.msg_id = 0x1C1D,
				.data = MQTT_SN_DATA_STRING_LITERAL("zephyr")
			}
		}
	},
	{
		MQTT_SN_ENCODE_TEST(puback),
		.p = {
			.type = MQTT_SN_MSG_TYPE_PUBACK,
			.params.puback = {
				.topic_id = 0x1A1B,
				.msg_id = 0x1C1D,
				.ret_code = MQTT_SN_CODE_REJECTED_NOTSUP
			}
		}
	},
	{
		MQTT_SN_ENCODE_TEST(pubrec),
		.p = {
			.type = MQTT_SN_MSG_TYPE_PUBREC,
			.params.pubrec = {
				.msg_id = 0xBEEF
			}
		}
	},
	{
		MQTT_SN_ENCODE_TEST(pubrel),
		.p = {
			.type = MQTT_SN_MSG_TYPE_PUBREL,
			.params.pubrel = {
				.msg_id = 0xBEEF
			}
		}
	},
	{
		MQTT_SN_ENCODE_TEST(pubcomp),
		.p = {
			.type = MQTT_SN_MSG_TYPE_PUBCOMP,
			.params.pubcomp = {
				.msg_id = 0xBEEF
			}
		}
	},
	{
		MQTT_SN_ENCODE_TEST(subscribe),
		.p = {
			.type = MQTT_SN_MSG_TYPE_SUBSCRIBE,
			.params.subscribe = {
				.dup = true,
				.qos = MQTT_SN_QOS_0,
				.msg_id = 0x1C1D,
				.topic_type = MQTT_SN_FLAGS_TOPICID_TYPE_NORMAL,
				.topic.topic_name = MQTT_SN_DATA_STRING_LITERAL("/zephyr")
			}
		}
	},
	{
		MQTT_SN_ENCODE_TEST(unsubscribe),
		.p = {
			.type = MQTT_SN_MSG_TYPE_UNSUBSCRIBE,
			.params.unsubscribe = {
				.msg_id = 0x1C1D,
				.topic_type = MQTT_SN_FLAGS_TOPICID_TYPE_PREDEF,
				.topic.topic_id = 0x1234
			}
		}
	},
	{
		MQTT_SN_ENCODE_TEST(pingreq),
		.p = {
			.type = MQTT_SN_MSG_TYPE_PINGREQ,
			.params.pingreq = {
				.client_id = MQTT_SN_DATA_STRING_LITERAL("zephyrclient")
			}
		}
	},
	{
		MQTT_SN_ENCODE_TEST(pingresp),
		.p = {
			.type = MQTT_SN_MSG_TYPE_PINGRESP
		}
	},
	{
		MQTT_SN_ENCODE_TEST(disconnect),
		.p = {
			.type = MQTT_SN_MSG_TYPE_DISCONNECT,
			.params.disconnect = {
				.duration = 10000
			}
		}
	},
	{
		MQTT_SN_ENCODE_TEST(willtopicupd),
		.p = {
			.type = MQTT_SN_MSG_TYPE_WILLTOPICUPD,
			.params.willtopicupd = {
				.qos = MQTT_SN_QOS_0,
				.retain = true,
				.topic = MQTT_SN_DATA_STRING_LITERAL("/zephyr")
			}
		}
	},
	{
		MQTT_SN_ENCODE_TEST(willmsgupd),
		.p = {
			.type = MQTT_SN_MSG_TYPE_WILLMSGUPD,
			.params.willmsgupd = {
				.msg = MQTT_SN_DATA_STRING_LITERAL("mywill")
			}
		}
	}
};

static ZTEST(mqtt_sn_packet, test_mqtt_packet_encode)
{
	NET_BUF_SIMPLE_DEFINE(msg, 255);
	int err;

	for (size_t i = 0; i < ARRAY_SIZE(encode_tests); i++) {
		net_buf_simple_reset(&msg);
		TC_PRINT("%s - test %zu: %s\n", __func__, i, encode_tests[i].name);

		err = mqtt_sn_encode_msg(&msg, &encode_tests[i].p);
		zassert_equal(err, 0, "Unexpected error");
		LOG_HEXDUMP_DBG(encode_tests[i].expected, encode_tests[i].expectedsz,
				"Expected data");
		LOG_HEXDUMP_DBG(msg.data, msg.len, "Encoded data");
		zassert_equal(msg.len, encode_tests[i].expectedsz,
			      "Unexpected data size %zu (%zu)", msg.len,
			      encode_tests[i].expectedsz);
		zassert_mem_equal(encode_tests[i].expected, msg.data, msg.len,
				  "Bad encoded message");
	}
}

ZTEST_SUITE(mqtt_sn_packet, NULL, NULL, NULL, NULL, NULL);
