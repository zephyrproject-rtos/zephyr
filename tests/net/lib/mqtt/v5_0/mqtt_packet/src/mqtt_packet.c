/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>

#include "mqtt_internal.h"

static uint8_t auth_data[] = { 0x01, 0x02, 0x03, 0x04 };
static uint8_t correlation_data[] = { 0x11, 0x12, 0x13, 0x14, 0x15, 0x16 };
static uint8_t test_payload[] = { 't', 'e', 's', 't', '_', 'p', 'a', 'y', 'l',
				  'o', 'a', 'd' };

#define BUFFER_SIZE 256
#define CLIENTID MQTT_UTF8_LITERAL("test_id")
#define WILL_TOPIC MQTT_UTF8_LITERAL("test_will")
#define WILL_MSG MQTT_UTF8_LITERAL("test_will_msg")
#define USER_PROP_NAME MQTT_UTF8_LITERAL("test_name")
#define USER_PROP_VALUE MQTT_UTF8_LITERAL("test_value")
#define AUTH_METHOD MQTT_UTF8_LITERAL("test_authentication")
#define AUTH_DATA (struct mqtt_binstr) { auth_data, sizeof(auth_data) }
#define CONTENT_TYPE MQTT_UTF8_LITERAL("test_content_type")
#define RESPONSE_TOPIC MQTT_UTF8_LITERAL("test_response_topic")
#define CORRELATION_DATA (struct mqtt_binstr) { correlation_data, sizeof(correlation_data) }
#define TEST_TOPIC MQTT_UTF8_LITERAL("test_topic")
#define TEST_PAYLOAD (struct mqtt_binstr) { test_payload, sizeof(test_payload) }
#define TEST_MSG_ID 0x1234

static uint8_t rx_buffer[BUFFER_SIZE];
static uint8_t tx_buffer[BUFFER_SIZE];
static struct mqtt_client client;

struct mqtt_test {
	/* Test name */
	const char *test_name;

	/* Test context, for example `struct mqtt_publish_param *` */
	void *ctx;

	/* Test function */
	void (*test_fcn)(struct mqtt_test *tests);

	/* Expected encoded packet */
	uint8_t *expected;

	/* Length packet length */
	uint16_t expected_len;
};

static void run_packet_tests(struct mqtt_test *tests, size_t count)
{
	for (size_t i = 0; i < count; i++) {
		struct mqtt_test *test = &tests[i];

		TC_PRINT("Test #%u - %s\n", i + 1, test->test_name != NULL ?
			test->test_name : "");

		test->test_fcn(test);
	};
}

static void print_buffer(const uint8_t *buf, uint16_t size)
{
	TC_PRINT("\n");
	for (uint16_t i = 0U; i < size; i++) {
		TC_PRINT("%02x ", buf[i]);
		if ((i + 1) % 8 == 0U) {
			TC_PRINT("\n");
		}
	}
	TC_PRINT("\n");
}

static int validate_buffers(const struct buf_ctx *buf, const uint8_t *expected,
			     uint16_t len)
{
	if (buf->end - buf->cur != len) {
		goto fail;
	}

	if (memcmp(expected, buf->cur, buf->end - buf->cur) != 0) {
		goto fail;
	}

	return 0;

fail:
	TC_PRINT("Computed:");
	print_buffer(buf->cur, buf->end - buf->cur);
	TC_PRINT("Expected:");
	print_buffer(expected, len);

	return -EBADMSG;
}

static int validate_structs(const void *computed, const void *expected,
			    size_t struct_size)
{
	if (memcmp(expected, computed, struct_size) != 0) {
		goto fail;
	}

	return 0;

fail:
	TC_PRINT("Computed:");
	print_buffer(computed, struct_size);
	TC_PRINT("Expected:");
	print_buffer(expected, struct_size);

	return -EBADMSG;
}

static void test_msg_connect(struct mqtt_test *test)
{
	struct mqtt_client *test_client;
	struct buf_ctx buf;
	int ret;

	test_client = (struct mqtt_client *)test->ctx;

	client.client_id = test_client->client_id;
	client.will_topic = test_client->will_topic;
	client.prop = test_client->prop;
	client.will_retain = test_client->will_retain;
	client.will_message = test_client->will_message;
	client.will_prop = test_client->will_prop;

	buf.cur = client.tx_buf;
	buf.end = client.tx_buf + client.tx_buf_size;

	ret = connect_request_encode(&client, &buf);
	zassert_ok(ret, "connect_request_encode failed");
	zassert_ok(validate_buffers(&buf, test->expected,
				    test->expected_len),
		   "Invalid packet content");
}

#define ENCODED_MID 0x12, 0x34
#define ENCODED_PROP_DEFAULT_TOPIC_ALIAS_MAXIMUM 0x22, 0x00, 0x05
#define ENCODED_PROP_DEFAULT_REQUEST_PROBLEM_INFORMATION 0x17, 0x00
#define ENCODED_PROP_USER_PROPERTY \
	0x26, 0x00, 0x09, 0x74, 0x65, 0x73, 0x74, 0x5f, 0x6e, 0x61, 0x6d, 0x65, \
	0x00, 0x0a, 0x74, 0x65, 0x73, 0x74, 0x5f, 0x76, 0x61, 0x6c, 0x75, 0x65
#define ENCODED_PROP_REASON_STRING \
	0x1f, 0x00, 0x12, 0x74, 0x65, 0x73, 0x74, 0x5F, 0x72, 0x65, 0x61, 0x73, \
	0x6F, 0x6E, 0x5F, 0x73, 0x74, 0x72, 0x69, 0x6E, 0x67
#define ENCODED_PROP_SESSION_EXPIRY_INTERVAL 0x11, 0x00, 0x00, 0x03, 0xe8
#define ENCODED_PROP_RECEIVE_MAXIMUM 0x21, 0x00, 0x0a
#define ENCODED_PROP_MAXIMUM_PACKET_SIZE 0x27, 0x00, 0x00, 0x03, 0xe8
#define ENCODED_PROP_REQUEST_RESPONSE_INFORMATION 0x19, 0x01
#define ENCODED_PROP_AUTHENTICATION_METHOD \
	0x15, 0x00, 0x13, 0x74, 0x65, 0x73, 0x74, 0x5f, 0x61, 0x75, 0x74, 0x68, \
	0x65, 0x6e, 0x74, 0x69, 0x63, 0x61, 0x74, 0x69, 0x6f, 0x6e
#define ENCODED_PROP_AUTHENTICATION_DATA \
	0x16, 0x00, 0x04, 0x01, 0x02, 0x03, 0x04
#define ENCODED_PROP_WILL_DELAY_INTERVAL 0x18, 0x00, 0x00, 0x00, 0x64
#define ENCODED_PROP_PAYLOAD_FORMAT_INDICATOR 0x01, 0x01
#define ENCODED_PROP_MESSAGE_EXPIRY_INTERVAL 0x02, 0x00, 0x00, 0x03, 0xe8
#define ENCODED_PROP_CONTENT_TYPE \
	0x03, 0x00, 0x11, 0x74, 0x65, 0x73, 0x74, 0x5f, 0x63, 0x6f, 0x6e, 0x74, \
	0x65, 0x6e, 0x74, 0x5f, 0x74, 0x79, 0x70, 0x65
#define ENCODED_PROP_RESPONSE_TOPIC \
	0x08, 0x00, 0x13, 0x74, 0x65, 0x73, 0x74, 0x5F, 0x72, 0x65, 0x73, 0x70, \
	0x6F, 0x6E, 0x73, 0x65, 0x5F, 0x74, 0x6F, 0x70, 0x69, 0x63
#define ENCODED_PROP_CORRELATION_DATA \
	0x09, 0x00, 0x06, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16
#define ENCODED_PROP_MAXIMUM_QOS 0x24, 0x01
#define ENCODED_PROP_RETAIN_AVAILABLE 0x25, 0x01
#define ENCODED_PROP_ASSIGNED_CLIENT_ID \
	0x12, 0x00, 0x07, 0x74, 0x65, 0x73, 0x74, 0x5f, 0x69, 0x64
#define ENCODED_PROP_TOPIC_ALIAS_MAXIMUM 0x22, 0x00, 0x0a
#define ENCODED_PROP_WILDCARD_SUB_AVAILABLE 0x28, 0x01
#define ENCODED_PROP_SUBSCRIPTION_IDS_AVAILABLE 0x29, 0x01
#define ENCODED_PROP_SHARED_SUB_AVAILABLE 0x2a, 0x01
#define ENCODED_PROP_SERVER_KEEP_ALIVE 0x13, 0x00, 0x64
#define ENCODED_PROP_RESPONSE_INFORMATION \
	0x1a, 0x00, 0x09, 0x74, 0x65, 0x73, 0x74, 0x5f, 0x69, 0x6e, 0x66, 0x6f
#define ENCODED_PROP_SERVER_REFERENCE \
	0x1c, 0x00, 0x0e, 0x74, 0x65, 0x73, 0x74, 0x5f, 0x72, 0x65, 0x66, 0x65, \
	0x72, 0x65, 0x6e, 0x63, 0x65
#define ENCODED_PROP_TOPIC_ALIAS 0x23, 0x00, 0x04
#define ENCODED_PROP_CORRELATION_DATA \
	0x09, 0x00, 0x06, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16
#define ENCODED_PROP_SUBSCRIPTION_IDENTIFIER 0x0b, 0xe8, 0x07

#define TEST_PROP_SESSION_EXPIRY_INTERVAL 1000
#define TEST_PROP_RECEIVE_MAXIMUM 10
#define TEST_PROP_MAXIMUM_PACKET_SIZE 1000
#define TEST_PROP_WILL_DELAY_INTERVAL 100
#define TEST_PROP_PAYLOAD_FORMAT_INDICATOR 1
#define TEST_PROP_MESSAGE_EXPIRY_INTERVAL 1000
#define TEST_PROP_MAXIMUM_QOS 1
#define TEST_PROP_RETAIN_AVAILABLE 1
#define TEST_PROP_TOPIC_ALIAS_MAXIMUM 10
#define TEST_PROP_WILDCARD_SUB_AVAILABLE 1
#define TEST_PROP_SUBSCRIPTION_IDS_AVAILABLE 1
#define TEST_PROP_SHARED_SUB_AVAILABLE 1
#define TEST_PROP_SERVER_KEEP_ALIVE 100
#define TEST_PROP_SUBSCRIPTION_IDENTIFIER 1000

#define ENCODED_CONNECT_VAR_HEADER_COMMON \
	0x00, 0x04, 0x4d, 0x51, 0x54, 0x54, 0x05, 0x00, 0x00, 0x3c
#define ENCODED_CONNECT_PROPERTIES_DEFAULT \
	0x05, 0x22, 0x00, 0x05, 0x17, 0x00
#define ENCODED_CONNECT_CLIENT_ID \
	0x00, 0x07, 0x74, 0x65, 0x73, 0x74, 0x5f, 0x69, 0x64

static uint8_t expect_connect_default[] = {
	0x10, 0x19, ENCODED_CONNECT_VAR_HEADER_COMMON,
	ENCODED_CONNECT_PROPERTIES_DEFAULT,
	ENCODED_CONNECT_CLIENT_ID,
};
static struct mqtt_client client_connect_default = {
	.client_id = CLIENTID,
};

static uint8_t expect_connect_prop_sei[] = {
	0x10, 0x1e, ENCODED_CONNECT_VAR_HEADER_COMMON,
	/* Properties */
	0x0a, ENCODED_PROP_SESSION_EXPIRY_INTERVAL,
	ENCODED_PROP_DEFAULT_TOPIC_ALIAS_MAXIMUM,
	ENCODED_PROP_DEFAULT_REQUEST_PROBLEM_INFORMATION,
	/* Payload */
	ENCODED_CONNECT_CLIENT_ID,
};
static struct mqtt_client client_connect_prop_sei = {
	.client_id = CLIENTID,
	.prop.session_expiry_interval = TEST_PROP_SESSION_EXPIRY_INTERVAL,
};

static uint8_t expect_connect_prop_rm[] = {
	0x10, 0x1c, ENCODED_CONNECT_VAR_HEADER_COMMON,
	/* Properties */
	0x08, ENCODED_PROP_RECEIVE_MAXIMUM,
	ENCODED_PROP_DEFAULT_TOPIC_ALIAS_MAXIMUM,
	ENCODED_PROP_DEFAULT_REQUEST_PROBLEM_INFORMATION,
	/* Payload */
	ENCODED_CONNECT_CLIENT_ID,
};
static struct mqtt_client client_connect_prop_rm = {
	.client_id = CLIENTID,
	.prop.receive_maximum = TEST_PROP_RECEIVE_MAXIMUM,
};

static uint8_t expect_connect_prop_mps[] = {
	0x10, 0x1e, ENCODED_CONNECT_VAR_HEADER_COMMON,
	/* Properties */
	0x0a, ENCODED_PROP_MAXIMUM_PACKET_SIZE,
	ENCODED_PROP_DEFAULT_TOPIC_ALIAS_MAXIMUM,
	ENCODED_PROP_DEFAULT_REQUEST_PROBLEM_INFORMATION,
	/* Payload */
	ENCODED_CONNECT_CLIENT_ID,

};
static struct mqtt_client client_connect_prop_mps = {
	.client_id = CLIENTID,
	.prop.maximum_packet_size = TEST_PROP_MAXIMUM_PACKET_SIZE,
};

static uint8_t expect_connect_prop_rri[] = {
	0x10, 0x1b, ENCODED_CONNECT_VAR_HEADER_COMMON,
	/* Properties */
	0x07, ENCODED_PROP_DEFAULT_TOPIC_ALIAS_MAXIMUM,
	ENCODED_PROP_REQUEST_RESPONSE_INFORMATION,
	ENCODED_PROP_DEFAULT_REQUEST_PROBLEM_INFORMATION,
	/* Payload */
	ENCODED_CONNECT_CLIENT_ID,
};
static struct mqtt_client client_connect_prop_rri = {
	.client_id = CLIENTID,
	.prop.request_response_info = true,
};

static uint8_t expect_connect_prop_rpi[] = {
	0x10, 0x17, ENCODED_CONNECT_VAR_HEADER_COMMON,
	/* Properties */
	0x03, ENCODED_PROP_DEFAULT_TOPIC_ALIAS_MAXIMUM,
	/* Payload */
	ENCODED_CONNECT_CLIENT_ID,
};
static struct mqtt_client client_connect_prop_rpi = {
	.client_id = CLIENTID,
	/* True is MQTT default so property should be omitted. */
	.prop.request_problem_info = true,
};

static uint8_t expect_connect_prop_up[] = {
	0x10, 0x31, ENCODED_CONNECT_VAR_HEADER_COMMON,
	/* Properties */
	0x1d, ENCODED_PROP_DEFAULT_TOPIC_ALIAS_MAXIMUM,
	ENCODED_PROP_DEFAULT_REQUEST_PROBLEM_INFORMATION,
	ENCODED_PROP_USER_PROPERTY,
	/* Payload */
	ENCODED_CONNECT_CLIENT_ID,
};
static struct mqtt_client client_connect_prop_up = {
	.client_id = CLIENTID,
	.prop.user_prop[0] = {
		.name = USER_PROP_NAME,
		.value = USER_PROP_VALUE,
	},
};

static uint8_t expect_connect_prop_am_ad[] = {
	0x10, 0x36, ENCODED_CONNECT_VAR_HEADER_COMMON,
	/* Properties */
	0x22, ENCODED_PROP_DEFAULT_TOPIC_ALIAS_MAXIMUM,
	ENCODED_PROP_DEFAULT_REQUEST_PROBLEM_INFORMATION,
	ENCODED_PROP_AUTHENTICATION_METHOD,
	ENCODED_PROP_AUTHENTICATION_DATA,
	/* Payload */
	ENCODED_CONNECT_CLIENT_ID,
};
static struct mqtt_client client_connect_prop_am_ad = {
	.client_id = CLIENTID,
	.prop.auth_method = AUTH_METHOD,
	.prop.auth_data = AUTH_DATA,
};

static struct mqtt_utf8 will_msg = WILL_MSG;
static struct mqtt_topic will_topic = {
	.qos = 0,
	.topic = WILL_TOPIC,
};

#define CONNECT_WILL_COMMON \
	.client_id = CLIENTID, \
	.will_topic = &will_topic, \
	.will_message = &will_msg \

#define ENCODED_CONNECT_WILL_VAR_HEADER_COMMON \
	0x00, 0x04, 0x4d, 0x51, 0x54, 0x54, 0x05, 0x04, 0x00, 0x3c
#define ENCODED_CONNECT_WILL_TOPIC_AND_PAYLOAD \
	0x00, 0x09, 0x74, 0x65, 0x73, 0x74, 0x5f, 0x77, 0x69, 0x6c, 0x6c, \
	0x00, 0x0d, 0x74, 0x65, 0x73, 0x74, 0x5f, 0x77, 0x69, 0x6c, 0x6c, 0x5f, \
	0x6d, 0x73, 0x67

static uint8_t expect_connect_will_default[] = {
	0x10, 0x34, ENCODED_CONNECT_WILL_VAR_HEADER_COMMON,
	ENCODED_CONNECT_PROPERTIES_DEFAULT,
	ENCODED_CONNECT_CLIENT_ID,
	/* Will properties */
	0x00,
	ENCODED_CONNECT_WILL_TOPIC_AND_PAYLOAD,
};
static struct mqtt_client client_connect_will_default = {
	CONNECT_WILL_COMMON,
};

static uint8_t expect_connect_will_prop_di[] = {
	0x10, 0x39, ENCODED_CONNECT_WILL_VAR_HEADER_COMMON,
	/* Properties */
	ENCODED_CONNECT_PROPERTIES_DEFAULT,
	/* Payload */
	ENCODED_CONNECT_CLIENT_ID,
	/* Will properties */
	0x05, ENCODED_PROP_WILL_DELAY_INTERVAL,
	ENCODED_CONNECT_WILL_TOPIC_AND_PAYLOAD,
};
static struct mqtt_client client_connect_will_prop_di = {
	CONNECT_WILL_COMMON,
	.will_prop.will_delay_interval = TEST_PROP_WILL_DELAY_INTERVAL,
};

static uint8_t expect_connect_will_prop_pfi[] = {
	0x10, 0x36, ENCODED_CONNECT_WILL_VAR_HEADER_COMMON,
	/* Properties */
	ENCODED_CONNECT_PROPERTIES_DEFAULT,
	/* Payload */
	ENCODED_CONNECT_CLIENT_ID,
	/* Will properties */
	0x02, ENCODED_PROP_PAYLOAD_FORMAT_INDICATOR,
	ENCODED_CONNECT_WILL_TOPIC_AND_PAYLOAD,
};
static struct mqtt_client client_connect_will_prop_pfi = {
	CONNECT_WILL_COMMON,
	.will_prop.payload_format_indicator = TEST_PROP_PAYLOAD_FORMAT_INDICATOR,
};

static uint8_t expect_connect_will_prop_mei[] = {
	0x10, 0x39, ENCODED_CONNECT_WILL_VAR_HEADER_COMMON,
	/* Properties */
	ENCODED_CONNECT_PROPERTIES_DEFAULT,
	/* Payload */
	ENCODED_CONNECT_CLIENT_ID,
	/* Will properties */
	0x05, ENCODED_PROP_MESSAGE_EXPIRY_INTERVAL,
	ENCODED_CONNECT_WILL_TOPIC_AND_PAYLOAD,
};
static struct mqtt_client client_connect_will_prop_mei = {
	CONNECT_WILL_COMMON,
	.will_prop.message_expiry_interval = TEST_PROP_MESSAGE_EXPIRY_INTERVAL,
};

static uint8_t expect_connect_will_prop_ct[] = {
	0x10, 0x48, ENCODED_CONNECT_WILL_VAR_HEADER_COMMON,
	/* Properties */
	ENCODED_CONNECT_PROPERTIES_DEFAULT,
	/* Payload */
	ENCODED_CONNECT_CLIENT_ID,
	/* Will properties */
	0x14, ENCODED_PROP_CONTENT_TYPE,
	ENCODED_CONNECT_WILL_TOPIC_AND_PAYLOAD,
};
static struct mqtt_client client_connect_will_prop_ct = {
	CONNECT_WILL_COMMON,
	.will_prop.content_type = CONTENT_TYPE,
};

static uint8_t expect_connect_will_prop_rt[] = {
	0x10, 0x4a, ENCODED_CONNECT_WILL_VAR_HEADER_COMMON,
	/* Properties */
	ENCODED_CONNECT_PROPERTIES_DEFAULT,
	/* Payload */
	ENCODED_CONNECT_CLIENT_ID,
	/* Will properties */
	0x16, ENCODED_PROP_RESPONSE_TOPIC,
	ENCODED_CONNECT_WILL_TOPIC_AND_PAYLOAD,
};
static struct mqtt_client client_connect_will_prop_rt = {
	CONNECT_WILL_COMMON,
	.will_prop.response_topic = RESPONSE_TOPIC,
};

static uint8_t expect_connect_will_prop_cd[] = {
	0x10, 0x3d, ENCODED_CONNECT_WILL_VAR_HEADER_COMMON,
	/* Properties */
	ENCODED_CONNECT_PROPERTIES_DEFAULT,
	/* Payload */
	ENCODED_CONNECT_CLIENT_ID,
	/* Will properties */
	0x09, ENCODED_PROP_CORRELATION_DATA,
	ENCODED_CONNECT_WILL_TOPIC_AND_PAYLOAD,
};
static struct mqtt_client client_connect_will_prop_cd = {
	CONNECT_WILL_COMMON,
	.will_prop.correlation_data = CORRELATION_DATA,
};

static uint8_t expect_connect_will_prop_up[] = {
	0x10, 0x4c, ENCODED_CONNECT_WILL_VAR_HEADER_COMMON,
	/* Properties */
	ENCODED_CONNECT_PROPERTIES_DEFAULT,
	/* Payload */
	ENCODED_CONNECT_CLIENT_ID,
	/* Will properties */
	0x18, ENCODED_PROP_USER_PROPERTY,
	ENCODED_CONNECT_WILL_TOPIC_AND_PAYLOAD,
};
static struct mqtt_client client_connect_will_prop_up = {
	CONNECT_WILL_COMMON,
	.will_prop.user_prop[0] = {
		.name = USER_PROP_NAME,
		.value = USER_PROP_VALUE,
	},
};

static struct mqtt_test connect_tests[] = {
	{ .test_name = "CONNECT, default",
	  .ctx = &client_connect_default, .test_fcn = test_msg_connect,
	  .expected = expect_connect_default,
	  .expected_len = sizeof(expect_connect_default) },
	{ .test_name = "CONNECT, property Session Expiry Interval",
	  .ctx = &client_connect_prop_sei, .test_fcn = test_msg_connect,
	  .expected = expect_connect_prop_sei,
	  .expected_len = sizeof(expect_connect_prop_sei) },
	{ .test_name = "CONNECT, property Receive Maximum",
	  .ctx = &client_connect_prop_rm, .test_fcn = test_msg_connect,
	  .expected = expect_connect_prop_rm,
	  .expected_len = sizeof(expect_connect_prop_rm) },
	{ .test_name = "CONNECT, property Maximum Packet Size",
	  .ctx = &client_connect_prop_mps, .test_fcn = test_msg_connect,
	  .expected = expect_connect_prop_mps,
	  .expected_len = sizeof(expect_connect_prop_mps) },
	{ .test_name = "CONNECT, property Request Response Information",
	  .ctx = &client_connect_prop_rri, .test_fcn = test_msg_connect,
	  .expected = expect_connect_prop_rri,
	  .expected_len = sizeof(expect_connect_prop_rri) },
	{ .test_name = "CONNECT, property Request Problem Information",
	  .ctx = &client_connect_prop_rpi, .test_fcn = test_msg_connect,
	  .expected = expect_connect_prop_rpi,
	  .expected_len = sizeof(expect_connect_prop_rpi) },
	{ .test_name = "CONNECT, property User Property",
	  .ctx = &client_connect_prop_up, .test_fcn = test_msg_connect,
	  .expected = expect_connect_prop_up,
	  .expected_len = sizeof(expect_connect_prop_up) },
	{ .test_name = "CONNECT, property Authentication Method and Data",
	  .ctx = &client_connect_prop_am_ad, .test_fcn = test_msg_connect,
	  .expected = expect_connect_prop_am_ad,
	  .expected_len = sizeof(expect_connect_prop_am_ad) },
	{ .test_name = "CONNECT, WILL default",
	  .ctx = &client_connect_will_default, .test_fcn = test_msg_connect,
	  .expected = expect_connect_will_default,
	  .expected_len = sizeof(expect_connect_will_default) },
	{ .test_name = "CONNECT, WILL property Will Delay Interval",
	  .ctx = &client_connect_will_prop_di, .test_fcn = test_msg_connect,
	  .expected = expect_connect_will_prop_di,
	  .expected_len = sizeof(expect_connect_will_prop_di) },
	{ .test_name = "CONNECT, WILL property Payload Format Indicator",
	  .ctx = &client_connect_will_prop_pfi, .test_fcn = test_msg_connect,
	  .expected = expect_connect_will_prop_pfi,
	  .expected_len = sizeof(expect_connect_will_prop_pfi) },
	{ .test_name = "CONNECT, WILL property Message Expiry Interval",
	  .ctx = &client_connect_will_prop_mei, .test_fcn = test_msg_connect,
	  .expected = expect_connect_will_prop_mei,
	  .expected_len = sizeof(expect_connect_will_prop_mei) },
	{ .test_name = "CONNECT, WILL property Content Type",
	  .ctx = &client_connect_will_prop_ct, .test_fcn = test_msg_connect,
	  .expected = expect_connect_will_prop_ct,
	  .expected_len = sizeof(expect_connect_will_prop_ct) },
	{ .test_name = "CONNECT, WILL property Response Topic",
	  .ctx = &client_connect_will_prop_rt, .test_fcn = test_msg_connect,
	  .expected = expect_connect_will_prop_rt,
	  .expected_len = sizeof(expect_connect_will_prop_rt) },
	{ .test_name = "CONNECT, WILL property Correlation Data",
	  .ctx = &client_connect_will_prop_cd, .test_fcn = test_msg_connect,
	  .expected = expect_connect_will_prop_cd,
	  .expected_len = sizeof(expect_connect_will_prop_cd) },
	{ .test_name = "CONNECT, WILL property User Property",
	  .ctx = &client_connect_will_prop_up, .test_fcn = test_msg_connect,
	  .expected = expect_connect_will_prop_up,
	  .expected_len = sizeof(expect_connect_will_prop_up) },
};

ZTEST(mqtt_5_packet, test_mqtt_5_connect)
{
	run_packet_tests(connect_tests, ARRAY_SIZE(connect_tests));
}

static void test_msg_connack(struct mqtt_test *test)
{
	struct mqtt_connack_param *exp_param =
			(struct mqtt_connack_param *)test->ctx;
	struct mqtt_connack_param dec_param = { 0 };
	int ret;
	uint8_t type_and_flags;
	uint32_t length;
	struct buf_ctx buf;

	buf.cur = test->expected;
	buf.end = test->expected + test->expected_len;

	ret = fixed_header_decode(&buf, &type_and_flags, &length);
	zassert_ok(ret, "fixed_header_decode failed");
	zassert_equal(type_and_flags & 0xF0, MQTT_PKT_TYPE_CONNACK,
		      "Invalid packet type");
	zassert_equal(length, test->expected_len - 2,
		      "Invalid packet length");

	ret = connect_ack_decode(&client, &buf, &dec_param);
	zassert_ok(ret, "connect_ack_decode failed");
	zassert_ok(validate_structs(&dec_param, exp_param, sizeof(dec_param)),
		   "Incorrect CONNACK params decoded");
}

static uint8_t expect_connack_default[] = {
	0x20, 0x03, 0x01, 0x00, 0x00,
};
static struct mqtt_connack_param connack_default = {
	.session_present_flag = true,
	.return_code = MQTT_CONNACK_SUCCESS,
};

static uint8_t expect_connack_error[] = {
	0x20, 0x03, 0x00, 0x80, 0x00,
};
static struct mqtt_connack_param connack_error = {
	.return_code = MQTT_CONNACK_UNSPECIFIED_ERROR,
};

static uint8_t expect_connack_prop_sei[] = {
	0x20, 0x08, 0x00, 0x00,
	/* Properties */
	0x05, ENCODED_PROP_SESSION_EXPIRY_INTERVAL,
};
static struct mqtt_connack_param connack_prop_sei = {
	.prop.rx.has_session_expiry_interval = true,
	.prop.session_expiry_interval = TEST_PROP_SESSION_EXPIRY_INTERVAL,
};

static uint8_t expect_connack_prop_rm[] = {
	0x20, 0x06, 0x00, 0x00,
	/* Properties */
	0x03, ENCODED_PROP_RECEIVE_MAXIMUM,
};
static struct mqtt_connack_param connack_prop_rm = {
	.prop.rx.has_receive_maximum = true,
	.prop.receive_maximum = TEST_PROP_RECEIVE_MAXIMUM,
};

static uint8_t expect_connack_prop_mqos[] = {
	0x20, 0x05, 0x00, 0x00,
	/* Properties */
	0x02, ENCODED_PROP_MAXIMUM_QOS,
};
static struct mqtt_connack_param connack_prop_mqos = {
	.prop.rx.has_maximum_qos = true,
	.prop.maximum_qos = TEST_PROP_MAXIMUM_QOS,
};

static uint8_t expect_connack_prop_ra[] = {
	0x20, 0x05, 0x00, 0x00,
	/* Properties */
	0x02, ENCODED_PROP_RETAIN_AVAILABLE,
};
static struct mqtt_connack_param connack_prop_ra = {
	.prop.rx.has_retain_available = true,
	.prop.retain_available = TEST_PROP_RETAIN_AVAILABLE,
};

static uint8_t expect_connack_prop_mps[] = {
	0x20, 0x08, 0x00, 0x00,
	/* Properties */
	0x05, ENCODED_PROP_MAXIMUM_PACKET_SIZE,
};
static struct mqtt_connack_param connack_prop_mps = {
	.prop.rx.has_maximum_packet_size = true,
	.prop.maximum_packet_size = TEST_PROP_MAXIMUM_PACKET_SIZE,
};

static uint8_t expect_connack_prop_aci[] = {
	0x20, 0x0d, 0x00, 0x00,
	/* Properties */
	0x0a, ENCODED_PROP_ASSIGNED_CLIENT_ID,
};
static struct mqtt_connack_param connack_prop_aci = {
	.prop.rx.has_assigned_client_id = true,
	.prop.assigned_client_id.utf8 = expect_connack_prop_aci + 8,
	.prop.assigned_client_id.size = 7,
};

static uint8_t expect_connack_prop_tam[] = {
	0x20, 0x06, 0x00, 0x00,
	/* Properties */
	0x03, ENCODED_PROP_TOPIC_ALIAS_MAXIMUM,
};
static struct mqtt_connack_param connack_prop_tam = {
	.prop.rx.has_topic_alias_maximum = true,
	.prop.topic_alias_maximum = TEST_PROP_TOPIC_ALIAS_MAXIMUM,
};

static uint8_t expect_connack_prop_rs[] = {
	0x20, 0x18, 0x00, 0x00,
	/* Properties */
	0x15, ENCODED_PROP_REASON_STRING,
};
static struct mqtt_connack_param connack_prop_rs = {
	.prop.rx.has_reason_string = true,
	.prop.reason_string.utf8 = expect_connack_prop_rs + 8,
	.prop.reason_string.size = 18,
};

static uint8_t expect_connack_prop_up[] = {
	0x20, 0x1b, 0x00, 0x00,
	/* Properties */
	0x18, ENCODED_PROP_USER_PROPERTY,
};
static struct mqtt_connack_param connack_prop_up = {
	.prop.rx.has_user_prop = true,
	.prop.user_prop[0].name.utf8 = expect_connack_prop_up + 8,
	.prop.user_prop[0].name.size = 9,
	.prop.user_prop[0].value.utf8 = expect_connack_prop_up + 19,
	.prop.user_prop[0].value.size = 10,
};

static uint8_t expect_connack_prop_wsa[] = {
	0x20, 0x05, 0x00, 0x00,
	/* Properties */
	0x02, ENCODED_PROP_WILDCARD_SUB_AVAILABLE,
};
static struct mqtt_connack_param connack_prop_wsa = {
	.prop.rx.has_wildcard_sub_available = true,
	.prop.wildcard_sub_available = TEST_PROP_WILDCARD_SUB_AVAILABLE,
};

static uint8_t expect_connack_prop_sia[] = {
	0x20, 0x05, 0x00, 0x00,
	/* Properties */
	0x02, ENCODED_PROP_SUBSCRIPTION_IDS_AVAILABLE,
};
static struct mqtt_connack_param connack_prop_sia = {
	.prop.rx.has_subscription_ids_available = true,
	.prop.subscription_ids_available = TEST_PROP_SUBSCRIPTION_IDS_AVAILABLE,
};

static uint8_t expect_connack_prop_ssa[] = {
	0x20, 0x05, 0x00, 0x00,
	/* Properties */
	0x02, ENCODED_PROP_SHARED_SUB_AVAILABLE,
};
static struct mqtt_connack_param connack_prop_ssa = {
	.prop.rx.has_shared_sub_available = true,
	.prop.shared_sub_available = TEST_PROP_SHARED_SUB_AVAILABLE,
};

static uint8_t expect_connack_prop_ska[] = {
	0x20, 0x06, 0x00, 0x00,
	/* Properties */
	0x03, ENCODED_PROP_SERVER_KEEP_ALIVE,
};
static struct mqtt_connack_param connack_prop_ska = {
	.prop.rx.has_server_keep_alive = true,
	.prop.server_keep_alive = TEST_PROP_SERVER_KEEP_ALIVE,
};

static uint8_t expect_connack_prop_ri[] = {
	0x20, 0x0f, 0x00, 0x00,
	/* Properties */
	0x0c, ENCODED_PROP_RESPONSE_INFORMATION,
};
static struct mqtt_connack_param connack_prop_ri = {
	.prop.rx.has_response_information = true,
	.prop.response_information.utf8 = expect_connack_prop_ri + 8,
	.prop.response_information.size = 9,
};

static uint8_t expect_connack_prop_sr[] = {
	0x20, 0x14, 0x00, 0x00,
	/* Properties */
	0x11, ENCODED_PROP_SERVER_REFERENCE,
};
static struct mqtt_connack_param connack_prop_sr = {
	.prop.rx.has_server_reference = true,
	.prop.server_reference.utf8 = expect_connack_prop_sr + 8,
	.prop.server_reference.size = 14,
};

static uint8_t expect_connack_prop_am_ad[] = {
	0x20, 0x20, 0x00, 0x00,
	/* Properties */
	0x1d, ENCODED_PROP_AUTHENTICATION_METHOD,
	ENCODED_PROP_AUTHENTICATION_DATA
};
static struct mqtt_connack_param connack_prop_am_ad = {
	.prop.rx.has_auth_method = true,
	.prop.auth_method.utf8 = expect_connack_prop_am_ad + 8,
	.prop.auth_method.size = 19,
	.prop.rx.has_auth_data = true,
	.prop.auth_data.data = expect_connack_prop_am_ad + 30,
	.prop.auth_data.len = 4,
};

static struct mqtt_test connack_tests[] = {
	{ .test_name = "CONNACK, default",
	  .ctx = &connack_default, .test_fcn = test_msg_connack,
	  .expected = expect_connack_default,
	  .expected_len = sizeof(expect_connack_default) },
	{ .test_name = "CONNACK, error",
	  .ctx = &connack_error, .test_fcn = test_msg_connack,
	  .expected = expect_connack_error,
	  .expected_len = sizeof(expect_connack_error) },
	{ .test_name = "CONNACK, property Session Expiry Interval",
	  .ctx = &connack_prop_sei, .test_fcn = test_msg_connack,
	  .expected = expect_connack_prop_sei,
	  .expected_len = sizeof(expect_connack_prop_sei) },
	{ .test_name = "CONNACK, property Receive Maximum",
	  .ctx = &connack_prop_rm, .test_fcn = test_msg_connack,
	  .expected = expect_connack_prop_rm,
	  .expected_len = sizeof(expect_connack_prop_rm) },
	{ .test_name = "CONNACK, property Maximum QoS",
	  .ctx = &connack_prop_mqos, .test_fcn = test_msg_connack,
	  .expected = expect_connack_prop_mqos,
	  .expected_len = sizeof(expect_connack_prop_mqos) },
	{ .test_name = "CONNACK, property Retain Available",
	  .ctx = &connack_prop_ra, .test_fcn = test_msg_connack,
	  .expected = expect_connack_prop_ra,
	  .expected_len = sizeof(expect_connack_prop_ra) },
	{ .test_name = "CONNACK, property Maximum Packet Size",
	  .ctx = &connack_prop_mps, .test_fcn = test_msg_connack,
	  .expected = expect_connack_prop_mps,
	  .expected_len = sizeof(expect_connack_prop_mps) },
	{ .test_name = "CONNACK, property Assigned Client Identifier",
	  .ctx = &connack_prop_aci, .test_fcn = test_msg_connack,
	  .expected = expect_connack_prop_aci,
	  .expected_len = sizeof(expect_connack_prop_aci) },
	{ .test_name = "CONNACK, property Topic Alias Maximum",
	  .ctx = &connack_prop_tam, .test_fcn = test_msg_connack,
	  .expected = expect_connack_prop_tam,
	  .expected_len = sizeof(expect_connack_prop_tam) },
	{ .test_name = "CONNACK, property Reason String",
	  .ctx = &connack_prop_rs, .test_fcn = test_msg_connack,
	  .expected = expect_connack_prop_rs,
	  .expected_len = sizeof(expect_connack_prop_rs) },
	{ .test_name = "CONNACK, property User Property",
	  .ctx = &connack_prop_up, .test_fcn = test_msg_connack,
	  .expected = expect_connack_prop_up,
	  .expected_len = sizeof(expect_connack_prop_up) },
	{ .test_name = "CONNACK, property Wildcard Subscription Available",
	  .ctx = &connack_prop_wsa, .test_fcn = test_msg_connack,
	  .expected = expect_connack_prop_wsa,
	  .expected_len = sizeof(expect_connack_prop_wsa) },
	{ .test_name = "CONNACK, property Subscription Identifiers Available",
	  .ctx = &connack_prop_sia, .test_fcn = test_msg_connack,
	  .expected = expect_connack_prop_sia,
	  .expected_len = sizeof(expect_connack_prop_sia) },
	{ .test_name = "CONNACK, property Shared Subscription Available",
	  .ctx = &connack_prop_ssa, .test_fcn = test_msg_connack,
	  .expected = expect_connack_prop_ssa,
	  .expected_len = sizeof(expect_connack_prop_ssa) },
	{ .test_name = "CONNACK, property Server Keep Alive",
	  .ctx = &connack_prop_ska, .test_fcn = test_msg_connack,
	  .expected = expect_connack_prop_ska,
	  .expected_len = sizeof(expect_connack_prop_ska) },
	{ .test_name = "CONNACK, property Response Information",
	  .ctx = &connack_prop_ri, .test_fcn = test_msg_connack,
	  .expected = expect_connack_prop_ri,
	  .expected_len = sizeof(expect_connack_prop_ri) },
	{ .test_name = "CONNACK, property Server Reference",
	  .ctx = &connack_prop_sr, .test_fcn = test_msg_connack,
	  .expected = expect_connack_prop_sr,
	  .expected_len = sizeof(expect_connack_prop_sr) },
	{ .test_name = "CONNACK, property Authentication Method and Data",
	  .ctx = &connack_prop_am_ad, .test_fcn = test_msg_connack,
	  .expected = expect_connack_prop_am_ad,
	  .expected_len = sizeof(expect_connack_prop_am_ad) },
};

ZTEST(mqtt_5_packet, test_mqtt_5_connack)
{
	run_packet_tests(connack_tests, ARRAY_SIZE(connack_tests));
}

static void test_msg_publish_dec_only(struct mqtt_test *test)
{
	struct mqtt_publish_param *exp_param =
			(struct mqtt_publish_param *)test->ctx;
	struct mqtt_publish_param dec_param = { 0 };
	uint8_t type_and_flags;
	uint32_t length;
	struct buf_ctx buf;
	int ret;

	buf.cur = test->expected;
	buf.end = test->expected + test->expected_len;

	ret = fixed_header_decode(&buf, &type_and_flags, &length);
	zassert_ok(ret, "fixed_header_decode failed");
	zassert_equal(type_and_flags & 0xF0, MQTT_PKT_TYPE_PUBLISH,
		      "Invalid packet type");
	zassert_equal(length, test->expected_len - 2,
		      "Invalid packet length");

	ret = publish_decode(&client, type_and_flags, length, &buf, &dec_param);
	zassert_ok(ret, "publish_decode failed");

	zassert_equal(dec_param.message_id, exp_param->message_id,
		      "Incorrect message_id");
	zassert_equal(dec_param.dup_flag, exp_param->dup_flag,
		      "Incorrect dup flag");
	zassert_equal(dec_param.retain_flag, exp_param->retain_flag,
		      "Incorrect retain flag");
	zassert_equal(dec_param.message.topic.qos, exp_param->message.topic.qos,
		      "Incorrect topic qos");
	zassert_equal(dec_param.message.topic.topic.size,
		      exp_param->message.topic.topic.size,
		      "Incorrect topic len");
	zassert_mem_equal(dec_param.message.topic.topic.utf8,
			  exp_param->message.topic.topic.utf8,
			  dec_param.message.topic.topic.size,
			  "Incorrect topic content");
	zassert_equal(dec_param.message.payload.len,
		      exp_param->message.payload.len,
		      "Incorrect payload len");
	zassert_ok(validate_structs(&dec_param.prop, &(exp_param->prop),
		   sizeof(dec_param.prop)),
		   "Incorrect PUBLISH properties decoded");
}

static void test_msg_publish(struct mqtt_test *test)
{
	struct mqtt_publish_param *exp_param =
			(struct mqtt_publish_param *)test->ctx;
	struct buf_ctx buf;
	int ret;

	buf.cur = client.tx_buf;
	buf.end = client.tx_buf + client.tx_buf_size;

	ret = publish_encode(&client, exp_param, &buf);
	zassert_ok(ret, "publish_encode failed");

	/* Payload is not copied, copy it manually just after the header.*/
	memcpy(buf.end, exp_param->message.payload.data,
		exp_param->message.payload.len);
	buf.end += exp_param->message.payload.len;

	zassert_ok(validate_buffers(&buf, test->expected,
				    test->expected_len),
		   "Invalid packet content");

	test_msg_publish_dec_only(test);
}

#define PUBLISH_COMMON \
	.message.topic.qos = MQTT_QOS_1_AT_LEAST_ONCE, \
	.message.topic.topic = TEST_TOPIC, \
	.message.payload = TEST_PAYLOAD, \
	.message_id = TEST_MSG_ID

#define ENCODED_PUBLISH_TOPIC \
	0x00, 0x0a, 0x74, 0x65, 0x73, 0x74, 0x5f, 0x74, 0x6f, 0x70, 0x69, 0x63
#define ENCODED_PUBLISH_PAYLOAD \
	0x74, 0x65, 0x73, 0x74, 0x5f, 0x70, 0x61, 0x79, 0x6c, 0x6f, 0x61, 0x64

static uint8_t expect_publish_default[] = {
	0x32, 0x1b, ENCODED_PUBLISH_TOPIC, ENCODED_MID,
	/* No properties */
	0x00,
	/* Payload */
	ENCODED_PUBLISH_PAYLOAD,
};
static struct mqtt_publish_param publish_default = {
	PUBLISH_COMMON,
};

static uint8_t expect_publish_prop_pfi[] = {
	0x32, 0x1d, ENCODED_PUBLISH_TOPIC, ENCODED_MID,
	/* Properties */
	0x02, ENCODED_PROP_PAYLOAD_FORMAT_INDICATOR,
	/* Payload */
	ENCODED_PUBLISH_PAYLOAD,
};
static struct mqtt_publish_param publish_prop_pfi = {
	PUBLISH_COMMON,
	.prop.rx.has_payload_format_indicator = true,
	.prop.payload_format_indicator = TEST_PROP_PAYLOAD_FORMAT_INDICATOR,
};

static uint8_t expect_publish_prop_mei[] = {
	0x32, 0x20, ENCODED_PUBLISH_TOPIC, ENCODED_MID,
	/* Properties */
	0x05, ENCODED_PROP_MESSAGE_EXPIRY_INTERVAL,
	/* Payload */
	ENCODED_PUBLISH_PAYLOAD,
};
static struct mqtt_publish_param publish_prop_mei = {
	PUBLISH_COMMON,
	.prop.rx.has_message_expiry_interval = true,
	.prop.message_expiry_interval = TEST_PROP_MESSAGE_EXPIRY_INTERVAL,
};

static uint8_t expect_publish_prop_ta[] = {
	0x32, 0x1e, ENCODED_PUBLISH_TOPIC, ENCODED_MID,
	/* Properties */
	0x03, ENCODED_PROP_TOPIC_ALIAS,
	/* Payload */
	ENCODED_PUBLISH_PAYLOAD,
};
static struct mqtt_publish_param publish_prop_ta = {
	PUBLISH_COMMON,
	.prop.rx.has_topic_alias = true,
	.prop.topic_alias = 4,
};

static uint8_t expect_publish_prop_rt[] = {
	0x32, 0x31, ENCODED_PUBLISH_TOPIC, ENCODED_MID,
	/* Properties */
	0x16, ENCODED_PROP_RESPONSE_TOPIC,
	/* Payload */
	ENCODED_PUBLISH_PAYLOAD,
};
static struct mqtt_publish_param publish_prop_rt = {
	PUBLISH_COMMON,
	.prop.rx.has_response_topic = true,
	.prop.response_topic.utf8 = expect_publish_prop_rt + 20,
	.prop.response_topic.size = 19,
};

static uint8_t expect_publish_prop_cd[] = {
	0x32, 0x24, ENCODED_PUBLISH_TOPIC, ENCODED_MID,
	/* Properties */
	0x09, ENCODED_PROP_CORRELATION_DATA,
	/* Payload */
	ENCODED_PUBLISH_PAYLOAD,
};
static struct mqtt_publish_param publish_prop_cd = {
	PUBLISH_COMMON,
	.prop.rx.has_correlation_data = true,
	.prop.correlation_data.data = expect_publish_prop_cd + 20,
	.prop.correlation_data.len = 6,
};

static uint8_t expect_publish_prop_up[] = {
	0x32, 0x33, ENCODED_PUBLISH_TOPIC, ENCODED_MID,
	/* Properties */
	0x18, ENCODED_PROP_USER_PROPERTY,
	/* Payload */
	ENCODED_PUBLISH_PAYLOAD,
};
static struct mqtt_publish_param publish_prop_up = {
	PUBLISH_COMMON,
	.prop.rx.has_user_prop = true,
	.prop.user_prop[0].name.utf8 = expect_publish_prop_up + 20,
	.prop.user_prop[0].name.size = 9,
	.prop.user_prop[0].value.utf8 = expect_publish_prop_up + 31,
	.prop.user_prop[0].value.size = 10,
};

static uint8_t expect_publish_prop_si[] = {
	0x32, 0x1e, ENCODED_PUBLISH_TOPIC, ENCODED_MID,
	/* Properties */
	0x03, ENCODED_PROP_SUBSCRIPTION_IDENTIFIER,
	/* Payload */
	ENCODED_PUBLISH_PAYLOAD,
};
static struct mqtt_publish_param publish_prop_si = {
	PUBLISH_COMMON,
	.prop.rx.has_subscription_identifier = true,
	.prop.subscription_identifier[0] = TEST_PROP_SUBSCRIPTION_IDENTIFIER,
};

static uint8_t expect_publish_prop_ct[] = {
	0x32, 0x2f, ENCODED_PUBLISH_TOPIC, ENCODED_MID,
	/* Properties */
	0x14, ENCODED_PROP_CONTENT_TYPE,
	/* Payload */
	ENCODED_PUBLISH_PAYLOAD,
};
static struct mqtt_publish_param publish_prop_ct = {
	PUBLISH_COMMON,
	.prop.rx.has_content_type = true,
	.prop.content_type.utf8 = expect_publish_prop_ct + 20,
	.prop.content_type.size = 17,
};

static struct mqtt_test publish_tests[] = {
	{ .test_name = "PUBLISH, default",
	  .ctx = &publish_default, .test_fcn = test_msg_publish,
	  .expected = expect_publish_default,
	  .expected_len = sizeof(expect_publish_default) },
	{ .test_name = "PUBLISH, property Payload Format Indicator",
	  .ctx = &publish_prop_pfi, .test_fcn = test_msg_publish,
	  .expected = expect_publish_prop_pfi,
	  .expected_len = sizeof(expect_publish_prop_pfi) },
	{ .test_name = "PUBLISH, property Message Expiry Interval",
	  .ctx = &publish_prop_mei, .test_fcn = test_msg_publish,
	  .expected = expect_publish_prop_mei,
	  .expected_len = sizeof(expect_publish_prop_mei) },
	{ .test_name = "PUBLISH, property Topic Alias",
	  .ctx = &publish_prop_ta, .test_fcn = test_msg_publish,
	  .expected = expect_publish_prop_ta,
	  .expected_len = sizeof(expect_publish_prop_ta) },
	{ .test_name = "PUBLISH, property Response Topic",
	  .ctx = &publish_prop_rt, .test_fcn = test_msg_publish,
	  .expected = expect_publish_prop_rt,
	  .expected_len = sizeof(expect_publish_prop_rt) },
	{ .test_name = "PUBLISH, property Correlation Data",
	  .ctx = &publish_prop_cd, .test_fcn = test_msg_publish,
	  .expected = expect_publish_prop_cd,
	  .expected_len = sizeof(expect_publish_prop_cd) },
	{ .test_name = "PUBLISH, property User Property",
	  .ctx = &publish_prop_up, .test_fcn = test_msg_publish,
	  .expected = expect_publish_prop_up,
	  .expected_len = sizeof(expect_publish_prop_up) },
	{ .test_name = "PUBLISH, property Subscription Identifier",
	  .ctx = &publish_prop_si, .test_fcn = test_msg_publish_dec_only,
	  .expected = expect_publish_prop_si,
	  .expected_len = sizeof(expect_publish_prop_si) },
	{ .test_name = "PUBLISH, property Content Type",
	  .ctx = &publish_prop_ct, .test_fcn = test_msg_publish,
	  .expected = expect_publish_prop_ct,
	  .expected_len = sizeof(expect_publish_prop_ct) },
};

ZTEST(mqtt_5_packet, test_mqtt_5_publish)
{
	run_packet_tests(publish_tests, ARRAY_SIZE(publish_tests));
}

static void test_msg_puback(struct mqtt_test *test)
{
	struct mqtt_puback_param *exp_param =
			(struct mqtt_puback_param *)test->ctx;
	struct mqtt_puback_param dec_param = { 0 };
	uint8_t type_and_flags;
	uint32_t length;
	struct buf_ctx buf;
	int ret;

	buf.cur = client.tx_buf;
	buf.end = client.tx_buf + client.tx_buf_size;

	ret = publish_ack_encode(&client, exp_param, &buf);
	zassert_ok(ret, "publish_ack_encode failed");
	zassert_ok(validate_buffers(&buf, test->expected,
				    test->expected_len),
		   "Invalid packet content");

	buf.cur = test->expected;
	buf.end = test->expected + test->expected_len;

	ret = fixed_header_decode(&buf, &type_and_flags, &length);
	zassert_ok(ret, "fixed_header_decode failed");
	zassert_equal(type_and_flags & 0xF0, MQTT_PKT_TYPE_PUBACK,
		      "Invalid packet type");
	zassert_equal(length, test->expected_len - 2,
		      "Invalid packet length");

	ret = publish_ack_decode(&client, &buf, &dec_param);
	zassert_ok(ret, "publish_ack_decode failed");
	zassert_ok(validate_structs(&dec_param, exp_param, sizeof(dec_param)),
		   "Incorrect PUBACK params decoded");
}

#define ENCODED_COMMON_ACK_DEFAULT 0x02, ENCODED_MID
#define COMMON_ACK_DEFAULT \
	.message_id = TEST_MSG_ID

static uint8_t expect_puback_default[] = {
	0x40, ENCODED_COMMON_ACK_DEFAULT,
};
static struct mqtt_puback_param puback_default = {
	COMMON_ACK_DEFAULT,
};

#define ENCODED_COMMON_ACK_ERROR 0x03, ENCODED_MID, 0x80
#define COMMON_ACK_ERROR \
	.message_id = TEST_MSG_ID, \
	.reason_code = 0x80

static uint8_t expect_puback_error[] = {
	0x40, ENCODED_COMMON_ACK_ERROR,
};
static struct mqtt_puback_param puback_error = {
	COMMON_ACK_ERROR,
};

#define ENCODED_COMMON_ACK_REASON_STRING \
	0x19, ENCODED_MID, 0x80, \
	0x15, ENCODED_PROP_REASON_STRING

#define COMMON_ACK_REASON_STRING(exp_payload) \
	.message_id = TEST_MSG_ID, \
	.reason_code = 0x80, \
	.prop.rx.has_reason_string = true, \
	.prop.reason_string.utf8 = exp_payload + 9, \
	.prop.reason_string.size = 18

static uint8_t expect_puback_prop_rs[] = {
	0x40, ENCODED_COMMON_ACK_REASON_STRING,
};
static struct mqtt_puback_param puback_prop_rs = {
	COMMON_ACK_REASON_STRING(expect_puback_prop_rs),
};

#define ENCODED_COMMON_ACK_USER_PROP \
	0x1c, ENCODED_MID, 0x00, \
	0x18, ENCODED_PROP_USER_PROPERTY

#define COMMON_ACK_USER_PROP(exp_payload) \
	.message_id = TEST_MSG_ID, \
	.prop.rx.has_user_prop = true, \
	.prop.user_prop[0].name.utf8 = exp_payload + 9, \
	.prop.user_prop[0].name.size = 9, \
	.prop.user_prop[0].value.utf8 = exp_payload + 20, \
	.prop.user_prop[0].value.size = 10

static uint8_t expect_puback_prop_up[] = {
	0x40, ENCODED_COMMON_ACK_USER_PROP,
};
static struct mqtt_puback_param puback_prop_up = {
	COMMON_ACK_USER_PROP(expect_puback_prop_up),
};

static struct mqtt_test puback_tests[] = {
	{ .test_name = "PUBACK, default",
	  .ctx = &puback_default, .test_fcn = test_msg_puback,
	  .expected = expect_puback_default,
	  .expected_len = sizeof(expect_puback_default) },
	{ .test_name = "PUBACK, error",
	  .ctx = &puback_error, .test_fcn = test_msg_puback,
	  .expected = expect_puback_error,
	  .expected_len = sizeof(expect_puback_error) },
	{ .test_name = "PUBACK, property Reason String",
	  .ctx = &puback_prop_rs, .test_fcn = test_msg_puback,
	  .expected = expect_puback_prop_rs,
	  .expected_len = sizeof(expect_puback_prop_rs) },
	{ .test_name = "PUBACK, property User Property",
	  .ctx = &puback_prop_up, .test_fcn = test_msg_puback,
	  .expected = expect_puback_prop_up,
	  .expected_len = sizeof(expect_puback_prop_up) },
};

ZTEST(mqtt_5_packet, test_mqtt_5_puback)
{
	run_packet_tests(puback_tests, ARRAY_SIZE(puback_tests));
}

static void test_msg_pubrec(struct mqtt_test *test)
{
	struct mqtt_pubrec_param *exp_param =
			(struct mqtt_pubrec_param *)test->ctx;
	struct mqtt_pubrec_param dec_param = { 0 };
	uint8_t type_and_flags;
	uint32_t length;
	struct buf_ctx buf;
	int ret;

	buf.cur = client.tx_buf;
	buf.end = client.tx_buf + client.tx_buf_size;

	ret = publish_receive_encode(&client, exp_param, &buf);
	zassert_ok(ret, "publish_receive_encode failed");
	zassert_ok(validate_buffers(&buf, test->expected,
				    test->expected_len),
		   "Invalid packet content");

	buf.cur = test->expected;
	buf.end = test->expected + test->expected_len;

	ret = fixed_header_decode(&buf, &type_and_flags, &length);
	zassert_ok(ret, "fixed_header_decode failed");
	zassert_equal(type_and_flags & 0xF0, MQTT_PKT_TYPE_PUBREC,
		      "Invalid packet type");
	zassert_equal(length, test->expected_len - 2,
		      "Invalid packet length");

	ret = publish_receive_decode(&client, &buf, &dec_param);
	zassert_ok(ret, "publish_receive_decode failed");
	zassert_ok(validate_structs(&dec_param, exp_param, sizeof(dec_param)),
		   "Incorrect PUBREC params decoded");
}

static uint8_t expect_pubrec_default[] = {
	0x50, ENCODED_COMMON_ACK_DEFAULT,
};
static struct mqtt_pubrec_param pubrec_default = {
	COMMON_ACK_DEFAULT,
};

static uint8_t expect_pubrec_error[] = {
	0x50, ENCODED_COMMON_ACK_ERROR,
};
static struct mqtt_pubrec_param pubrec_error = {
	COMMON_ACK_ERROR,
};

static uint8_t expect_pubrec_prop_rs[] = {
	0x50, ENCODED_COMMON_ACK_REASON_STRING,
};
static struct mqtt_pubrec_param pubrec_prop_rs = {
	COMMON_ACK_REASON_STRING(expect_pubrec_prop_rs),
};

static uint8_t expect_pubrec_prop_up[] = {
	0x50, ENCODED_COMMON_ACK_USER_PROP,
};
static struct mqtt_pubrec_param pubrec_prop_up = {
	COMMON_ACK_USER_PROP(expect_pubrec_prop_up),
};

static struct mqtt_test pubrec_tests[] = {
	{ .test_name = "PUBREC, default",
	  .ctx = &pubrec_default, .test_fcn = test_msg_pubrec,
	  .expected = expect_pubrec_default,
	  .expected_len = sizeof(expect_pubrec_default) },
	{ .test_name = "PUBREC, error",
	  .ctx = &pubrec_error, .test_fcn = test_msg_pubrec,
	  .expected = expect_pubrec_error,
	  .expected_len = sizeof(expect_pubrec_error) },
	{ .test_name = "PUBREC, property Reason String",
	  .ctx = &pubrec_prop_rs, .test_fcn = test_msg_pubrec,
	  .expected = expect_pubrec_prop_rs,
	  .expected_len = sizeof(expect_pubrec_prop_rs) },
	{ .test_name = "PUBREC, property User Property",
	  .ctx = &pubrec_prop_up, .test_fcn = test_msg_pubrec,
	  .expected = expect_pubrec_prop_up,
	  .expected_len = sizeof(expect_pubrec_prop_up) },
};

ZTEST(mqtt_5_packet, test_mqtt_5_pubrec)
{
	run_packet_tests(pubrec_tests, ARRAY_SIZE(pubrec_tests));
}

static void test_msg_pubrel(struct mqtt_test *test)
{
	struct mqtt_pubrel_param *exp_param =
			(struct mqtt_pubrel_param *)test->ctx;
	struct mqtt_pubrel_param dec_param = { 0 };
	uint8_t type_and_flags;
	uint32_t length;
	struct buf_ctx buf;
	int ret;

	buf.cur = client.tx_buf;
	buf.end = client.tx_buf + client.tx_buf_size;

	ret = publish_release_encode(&client, exp_param, &buf);
	zassert_ok(ret, "publish_release_encode failed");
	zassert_ok(validate_buffers(&buf, test->expected,
				    test->expected_len),
		   "Invalid packet content");

	buf.cur = test->expected;
	buf.end = test->expected + test->expected_len;

	ret = fixed_header_decode(&buf, &type_and_flags, &length);
	zassert_ok(ret, "fixed_header_decode failed");
	zassert_equal(type_and_flags & 0xF0, MQTT_PKT_TYPE_PUBREL,
		      "Invalid packet type");
	zassert_equal(length, test->expected_len - 2,
		      "Invalid packet length");

	ret = publish_release_decode(&client, &buf, &dec_param);
	zassert_ok(ret, "publish_release_decode failed");
	zassert_ok(validate_structs(&dec_param, exp_param, sizeof(dec_param)),
		   "Incorrect PUBREL params decoded");
}

static uint8_t expect_pubrel_default[] = {
	0x62, ENCODED_COMMON_ACK_DEFAULT,
};
static struct mqtt_pubrel_param pubrel_default = {
	COMMON_ACK_DEFAULT,
};

static uint8_t expect_pubrel_error[] = {
	0x62, ENCODED_COMMON_ACK_ERROR,
};
static struct mqtt_pubrel_param pubrel_error = {
	COMMON_ACK_ERROR,
};

static uint8_t expect_pubrel_prop_rs[] = {
	0x62, ENCODED_COMMON_ACK_REASON_STRING,
};
static struct mqtt_pubrel_param pubrel_prop_rs = {
	COMMON_ACK_REASON_STRING(expect_pubrel_prop_rs),
};

static uint8_t expect_pubrel_prop_up[] = {
	0x62, ENCODED_COMMON_ACK_USER_PROP,
};
static struct mqtt_pubrel_param pubrel_prop_up = {
	COMMON_ACK_USER_PROP(expect_pubrel_prop_up),
};

static struct mqtt_test pubrel_tests[] = {
	{ .test_name = "PUBREL, default",
	  .ctx = &pubrel_default, .test_fcn = test_msg_pubrel,
	  .expected = expect_pubrel_default,
	  .expected_len = sizeof(expect_pubrel_default) },
	{ .test_name = "PUBREL, error",
	  .ctx = &pubrel_error, .test_fcn = test_msg_pubrel,
	  .expected = expect_pubrel_error,
	  .expected_len = sizeof(expect_pubrel_error) },
	{ .test_name = "PUBREL, property Reason String",
	  .ctx = &pubrel_prop_rs, .test_fcn = test_msg_pubrel,
	  .expected = expect_pubrel_prop_rs,
	  .expected_len = sizeof(expect_pubrel_prop_rs) },
	{ .test_name = "PUBREL, property User Property",
	  .ctx = &pubrel_prop_up, .test_fcn = test_msg_pubrel,
	  .expected = expect_pubrel_prop_up,
	  .expected_len = sizeof(expect_pubrel_prop_up) },
};

ZTEST(mqtt_5_packet, test_mqtt_5_pubrel)
{
	run_packet_tests(pubrel_tests, ARRAY_SIZE(pubrel_tests));
}

static void test_msg_pubcomp(struct mqtt_test *test)
{
	struct mqtt_pubcomp_param *exp_param =
			(struct mqtt_pubcomp_param *)test->ctx;
	struct mqtt_pubcomp_param dec_param = { 0 };
	uint8_t type_and_flags;
	uint32_t length;
	struct buf_ctx buf;
	int ret;

	buf.cur = client.tx_buf;
	buf.end = client.tx_buf + client.tx_buf_size;

	ret = publish_complete_encode(&client, exp_param, &buf);
	zassert_ok(ret, "publish_complete_encode failed");
	zassert_ok(validate_buffers(&buf, test->expected,
				    test->expected_len),
		   "Invalid packet content");

	buf.cur = test->expected;
	buf.end = test->expected + test->expected_len;

	ret = fixed_header_decode(&buf, &type_and_flags, &length);
	zassert_ok(ret, "fixed_header_decode failed");
	zassert_equal(type_and_flags & 0xF0, MQTT_PKT_TYPE_PUBCOMP,
		      "Invalid packet type");
	zassert_equal(length, test->expected_len - 2,
		      "Invalid packet length");

	ret = publish_complete_decode(&client, &buf, &dec_param);
	zassert_ok(ret, "publish_complete_decode failed");
	zassert_ok(validate_structs(&dec_param, exp_param, sizeof(dec_param)),
		   "Incorrect PUBCOMP params decoded");
}

static uint8_t expect_pubcomp_default[] = {
	0x70, ENCODED_COMMON_ACK_DEFAULT,
};
static struct mqtt_pubcomp_param pubcomp_default = {
	COMMON_ACK_DEFAULT,
};

static uint8_t expect_pubcomp_error[] = {
	0x70, ENCODED_COMMON_ACK_ERROR,
};
static struct mqtt_pubcomp_param pubcomp_error = {
	COMMON_ACK_ERROR,
};

static uint8_t expect_pubcomp_prop_rs[] = {
	0x70, ENCODED_COMMON_ACK_REASON_STRING,
};
static struct mqtt_pubcomp_param pubcomp_prop_rs = {
	COMMON_ACK_REASON_STRING(expect_pubcomp_prop_rs),
};

static uint8_t expect_pubcomp_prop_up[] = {
	0x70, ENCODED_COMMON_ACK_USER_PROP,
};
static struct mqtt_pubcomp_param pubcomp_prop_up = {
	COMMON_ACK_USER_PROP(expect_pubcomp_prop_up),
};

static struct mqtt_test pubcomp_tests[] = {
	{ .test_name = "PUBCOMP, default",
	  .ctx = &pubcomp_default, .test_fcn = test_msg_pubcomp,
	  .expected = expect_pubcomp_default,
	  .expected_len = sizeof(expect_pubcomp_default) },
	{ .test_name = "PUBCOMP, error",
	  .ctx = &pubcomp_error, .test_fcn = test_msg_pubcomp,
	  .expected = expect_pubcomp_error,
	  .expected_len = sizeof(expect_pubcomp_error) },
	{ .test_name = "PUBCOMP, property Reason String",
	  .ctx = &pubcomp_prop_rs, .test_fcn = test_msg_pubcomp,
	  .expected = expect_pubcomp_prop_rs,
	  .expected_len = sizeof(expect_pubcomp_prop_rs) },
	{ .test_name = "PUBCOMP, property User Property",
	  .ctx = &pubcomp_prop_up, .test_fcn = test_msg_pubcomp,
	  .expected = expect_pubcomp_prop_up,
	  .expected_len = sizeof(expect_pubcomp_prop_up) },
};

ZTEST(mqtt_5_packet, test_mqtt_5_pubcomp)
{
	run_packet_tests(pubcomp_tests, ARRAY_SIZE(pubcomp_tests));
}

static void test_msg_subscribe(struct mqtt_test *test)
{
	struct mqtt_subscription_list *exp_param =
		(struct mqtt_subscription_list *)test->ctx;
	struct buf_ctx buf;
	int ret;

	buf.cur = client.tx_buf;
	buf.end = client.tx_buf + client.tx_buf_size;

	ret = subscribe_encode(&client, exp_param, &buf);
	zassert_ok(ret, "subscribe_encode failed");
	zassert_ok(validate_buffers(&buf, test->expected,
				    test->expected_len),
		   "Invalid packet content");
}

static struct mqtt_topic test_sub_topic = {
	.qos = MQTT_QOS_1_AT_LEAST_ONCE,
	.topic = TEST_TOPIC,
};

#define SUBSCRIBE_COMMON \
	.list = &test_sub_topic, \
	.list_count = 1, \
	.message_id = TEST_MSG_ID

#define ENCODED_SUBSCRIBE_TOPIC \
	0x00, 0x0a, 0x74, 0x65, 0x73, 0x74, 0x5f, 0x74, 0x6f, 0x70, 0x69, 0x63, \
	0x01

static uint8_t expect_subscribe_default[] = {
	0x82, 0x10, ENCODED_MID,
	/* No properties */
	0x00,
	/* Payload */
	ENCODED_SUBSCRIBE_TOPIC,
};
static struct mqtt_subscription_list subscribe_default = {
	SUBSCRIBE_COMMON,
};

static uint8_t expect_subscribe_prop_si[] = {
	0x82, 0x13, ENCODED_MID,
	/* Properties */
	0x03, ENCODED_PROP_SUBSCRIPTION_IDENTIFIER,
	/* Payload */
	ENCODED_SUBSCRIBE_TOPIC,
};
static struct mqtt_subscription_list subscribe_prop_si = {
	SUBSCRIBE_COMMON,
	.prop.subscription_identifier = 1000,
};

static uint8_t expect_subscribe_prop_up[] = {
	0x82, 0x28, ENCODED_MID,
	/* Properties */
	0x18, ENCODED_PROP_USER_PROPERTY,
	/* Payload */
	ENCODED_SUBSCRIBE_TOPIC,
};
static struct mqtt_subscription_list subscribe_prop_up = {
	SUBSCRIBE_COMMON,
	.prop.user_prop[0] = {
		.name = USER_PROP_NAME,
		.value = USER_PROP_VALUE,
	},
};

static struct mqtt_test subscribe_tests[] = {
	{ .test_name = "SUBSCRIBE, default",
	  .ctx = &subscribe_default, .test_fcn = test_msg_subscribe,
	  .expected = expect_subscribe_default,
	  .expected_len = sizeof(expect_subscribe_default) },
	{ .test_name = "SUBSCRIBE, property Subscription Identifier",
	  .ctx = &subscribe_prop_si, .test_fcn = test_msg_subscribe,
	  .expected = expect_subscribe_prop_si,
	  .expected_len = sizeof(expect_subscribe_prop_si) },
	{ .test_name = "SUBSCRIBE, property User Property",
	  .ctx = &subscribe_prop_up, .test_fcn = test_msg_subscribe,
	  .expected = expect_subscribe_prop_up,
	  .expected_len = sizeof(expect_subscribe_prop_up) },
};

ZTEST(mqtt_5_packet, test_mqtt_5_subscribe)
{
	run_packet_tests(subscribe_tests, ARRAY_SIZE(subscribe_tests));
}

static void test_msg_suback(struct mqtt_test *test)
{
	struct mqtt_suback_param *exp_param =
			(struct mqtt_suback_param *)test->ctx;
	struct mqtt_suback_param dec_param = { 0 };
	int ret;
	uint8_t type_and_flags;
	uint32_t length;
	struct buf_ctx buf;

	buf.cur = test->expected;
	buf.end = test->expected + test->expected_len;

	ret = fixed_header_decode(&buf, &type_and_flags, &length);
	zassert_ok(ret, "fixed_header_decode failed");
	zassert_equal(type_and_flags & 0xF0, MQTT_PKT_TYPE_SUBACK,
		      "Invalid packet type");
	zassert_equal(length, test->expected_len - 2,
		      "Invalid packet length");

	ret = subscribe_ack_decode(&client, &buf, &dec_param);
	zassert_ok(ret, "subscribe_ack_decode failed");
	zassert_ok(validate_structs(&dec_param, exp_param, sizeof(dec_param)),
		   "Incorrect SUBACK params decoded");
}

static uint8_t expect_suback_default[] = {
	0x90, 0x05, ENCODED_MID,
	/* No properties */
	0x00,
	/* Payload */
	0x01, 0x02,
};
static struct mqtt_suback_param suback_default = {
	.message_id = TEST_MSG_ID,
	.return_codes.data = expect_suback_default + 5,
	.return_codes.len = 2,
};

static uint8_t expect_suback_prop_rs[] = {
	0x90, 0x1a, ENCODED_MID,
	/* Properties */
	0x15, ENCODED_PROP_REASON_STRING,
	/* Payload */
	0x01, 0x02,
};
static struct mqtt_suback_param suback_prop_rs = {
	.message_id = TEST_MSG_ID,
	.prop.rx.has_reason_string = true,
	.prop.reason_string.utf8 = expect_suback_prop_rs + 8,
	.prop.reason_string.size = 18,
	.return_codes.data = expect_suback_prop_rs + 26,
	.return_codes.len = 2,
};

static uint8_t expect_suback_prop_up[] = {
	0x90, 0x1d, ENCODED_MID,
	/* Properties */
	0x18, ENCODED_PROP_USER_PROPERTY,
	/* Payload */
	0x01, 0x02,
};
static struct mqtt_suback_param suback_prop_up = {
	.message_id = TEST_MSG_ID,
	.prop.rx.has_user_prop = true,
	.prop.user_prop[0].name.utf8 = expect_suback_prop_up + 8,
	.prop.user_prop[0].name.size = 9,
	.prop.user_prop[0].value.utf8 = expect_suback_prop_up + 19,
	.prop.user_prop[0].value.size = 10,
	.return_codes.data = expect_suback_prop_up + 29,
	.return_codes.len = 2,
};

static struct mqtt_test suback_tests[] = {
	{ .test_name = "SUBACK, default",
	  .ctx = &suback_default, .test_fcn = test_msg_suback,
	  .expected = expect_suback_default,
	  .expected_len = sizeof(expect_suback_default) },
	{ .test_name = "SUBACK, property Reason String",
	  .ctx = &suback_prop_rs, .test_fcn = test_msg_suback,
	  .expected = expect_suback_prop_rs,
	  .expected_len = sizeof(expect_suback_prop_rs) },
	{ .test_name = "SUBACK, property User Property",
	  .ctx = &suback_prop_up, .test_fcn = test_msg_suback,
	  .expected = expect_suback_prop_up,
	  .expected_len = sizeof(expect_suback_prop_up) },
};

ZTEST(mqtt_5_packet, test_mqtt_5_suback)
{
	run_packet_tests(suback_tests, ARRAY_SIZE(suback_tests));
}

static void test_msg_unsubscribe(struct mqtt_test *test)
{
	struct mqtt_subscription_list *exp_param =
		(struct mqtt_subscription_list *)test->ctx;
	struct buf_ctx buf;
	int ret;

	buf.cur = client.tx_buf;
	buf.end = client.tx_buf + client.tx_buf_size;

	ret = unsubscribe_encode(&client, exp_param, &buf);
	zassert_ok(ret, "unsubscribe_encode failed");
	zassert_ok(validate_buffers(&buf, test->expected,
				    test->expected_len),
		   "Invalid packet content");
}

static struct mqtt_topic test_unsub_topic = {
	.qos = MQTT_QOS_1_AT_LEAST_ONCE,
	.topic = TEST_TOPIC,
};

#define UNSUBSCRIBE_COMMON \
	.list = &test_unsub_topic, \
	.list_count = 1, \
	.message_id = TEST_MSG_ID

#define ENCODED_UNSUBSCRIBE_TOPIC \
	0x00, 0x0a, 0x74, 0x65, 0x73, 0x74, 0x5f, 0x74, 0x6f, 0x70, 0x69, 0x63

static uint8_t expect_unsubscribe_default[] = {
	0xa2, 0x0f, ENCODED_MID,
	/* No properties */
	0x00,
	/* Payload */
	ENCODED_UNSUBSCRIBE_TOPIC,
};
static struct mqtt_subscription_list unsubscribe_default = {
	UNSUBSCRIBE_COMMON,
};

static uint8_t expect_unsubscribe_prop_up[] = {
	0xa2, 0x27, ENCODED_MID,
	/* Properties */
	0x18, ENCODED_PROP_USER_PROPERTY,
	/* Payload */
	ENCODED_UNSUBSCRIBE_TOPIC,
};
static struct mqtt_subscription_list unsubscribe_prop_up = {
	UNSUBSCRIBE_COMMON,
	.prop.user_prop[0] = {
		.name = USER_PROP_NAME,
		.value = USER_PROP_VALUE,
	},
};

static struct mqtt_test unsubscribe_tests[] = {
	{ .test_name = "UNSUBSCRIBE, default",
	  .ctx = &unsubscribe_default, .test_fcn = test_msg_unsubscribe,
	  .expected = expect_unsubscribe_default,
	  .expected_len = sizeof(expect_unsubscribe_default) },
	{ .test_name = "UNSUBSCRIBE, property User Property",
	  .ctx = &unsubscribe_prop_up, .test_fcn = test_msg_unsubscribe,
	  .expected = expect_unsubscribe_prop_up,
	  .expected_len = sizeof(expect_unsubscribe_prop_up) },
};

ZTEST(mqtt_5_packet, test_mqtt_5_unsubscribe)
{
	run_packet_tests(unsubscribe_tests, ARRAY_SIZE(unsubscribe_tests));
}

static void test_msg_unsuback(struct mqtt_test *test)
{
	struct mqtt_unsuback_param *exp_param =
			(struct mqtt_unsuback_param *)test->ctx;
	struct mqtt_unsuback_param dec_param = { 0 };
	int ret;
	uint8_t type_and_flags;
	uint32_t length;
	struct buf_ctx buf;

	buf.cur = test->expected;
	buf.end = test->expected + test->expected_len;

	ret = fixed_header_decode(&buf, &type_and_flags, &length);
	zassert_ok(ret, "fixed_header_decode failed");
	zassert_equal(type_and_flags & 0xF0, MQTT_PKT_TYPE_UNSUBACK,
		      "Invalid packet type");
	zassert_equal(length, test->expected_len - 2,
		      "Invalid packet length");

	ret = unsubscribe_ack_decode(&client, &buf, &dec_param);
	zassert_ok(ret, "unsubscribe_ack_decode failed");
	zassert_ok(validate_structs(&dec_param, exp_param, sizeof(dec_param)),
		   "Incorrect UNSUBACK params decoded");
}

static uint8_t expect_unsuback_default[] = {
	0xb0, 0x05, ENCODED_MID,
	/* No properties */
	0x00,
	/* Payload */
	0x00, 0x00,
};
static struct mqtt_unsuback_param unsuback_default = {
	.message_id = TEST_MSG_ID,
	.reason_codes.data = expect_unsuback_default + 5,
	.reason_codes.len = 2,
};

static uint8_t expect_unsuback_prop_rs[] = {
	0xb0, 0x19, ENCODED_MID,
	/* Properties */
	0x15, ENCODED_PROP_REASON_STRING,
	/* Payload */
	0x00,
};
static struct mqtt_unsuback_param unsuback_prop_rs = {
	.message_id = TEST_MSG_ID,
	.prop.rx.has_reason_string = true,
	.prop.reason_string.utf8 = expect_unsuback_prop_rs + 8,
	.prop.reason_string.size = 18,
	.reason_codes.data = expect_unsuback_prop_rs + 26,
	.reason_codes.len = 1,
};

static uint8_t expect_unsuback_prop_up[] = {
	0xb0, 0x1c, ENCODED_MID,
	/* Properties */
	0x18, ENCODED_PROP_USER_PROPERTY,
	/* Payload */
	0x00,
};
static struct mqtt_unsuback_param unsuback_prop_up = {
	.message_id = TEST_MSG_ID,
	.prop.rx.has_user_prop = true,
	.prop.user_prop[0].name.utf8 = expect_unsuback_prop_up + 8,
	.prop.user_prop[0].name.size = 9,
	.prop.user_prop[0].value.utf8 = expect_unsuback_prop_up + 19,
	.prop.user_prop[0].value.size = 10,
	.reason_codes.data = expect_unsuback_prop_up + 29,
	.reason_codes.len = 1,
};

static struct mqtt_test unsuback_tests[] = {
	{ .test_name = "UNSUBACK, default",
	  .ctx = &unsuback_default, .test_fcn = test_msg_unsuback,
	  .expected = expect_unsuback_default,
	  .expected_len = sizeof(expect_unsuback_default) },
	{ .test_name = "UNSUBACK, property Reason String",
	  .ctx = &unsuback_prop_rs, .test_fcn = test_msg_unsuback,
	  .expected = expect_unsuback_prop_rs,
	  .expected_len = sizeof(expect_unsuback_prop_rs) },
	{ .test_name = "UNSUBACK, property User Property",
	  .ctx = &unsuback_prop_up, .test_fcn = test_msg_unsuback,
	  .expected = expect_unsuback_prop_up,
	  .expected_len = sizeof(expect_unsuback_prop_up) },
};

ZTEST(mqtt_5_packet, test_mqtt_5_unsuback)
{
	run_packet_tests(unsuback_tests, ARRAY_SIZE(unsuback_tests));
}

static void test_msg_disconnect(struct mqtt_test *test)
{
	struct mqtt_disconnect_param *exp_param =
			(struct mqtt_disconnect_param *)test->ctx;
	struct mqtt_disconnect_param dec_param = { 0 };
	uint8_t type_and_flags;
	struct buf_ctx buf;
	uint32_t length;
	int ret;

	buf.cur = client.tx_buf;
	buf.end = client.tx_buf + client.tx_buf_size;

	ret = disconnect_encode(&client, exp_param, &buf);
	zassert_ok(ret, "disconnect_encode failed");
	zassert_ok(validate_buffers(&buf, test->expected,
				    test->expected_len),
		   "Invalid packet content");

	buf.cur = test->expected;
	buf.end = test->expected + test->expected_len;

	ret = fixed_header_decode(&buf, &type_and_flags, &length);
	zassert_ok(ret, "fixed_header_decode failed");
	zassert_equal(type_and_flags & 0xF0, MQTT_PKT_TYPE_DISCONNECT,
		      "Invalid packet type");
	zassert_equal(length, test->expected_len - 2,
		      "Invalid packet length");

	ret = disconnect_decode(&client, &buf, &dec_param);
	zassert_ok(ret, "disconnect_decode failed");
	zassert_ok(validate_structs(&dec_param, exp_param, sizeof(dec_param)),
		   "Incorrect DISCONNECT params decoded");
}

static uint8_t expect_disconnect_default[] = {
	0xe0, 0x00,
};
static struct mqtt_disconnect_param disconnect_default = {
	.reason_code = MQTT_DISCONNECT_NORMAL,
};

static uint8_t expect_disconnect_error[] = {
	0xe0, 0x01, 0x82,
};
static struct mqtt_disconnect_param disconnect_error = {
	.reason_code = MQTT_DISCONNECT_PROTOCOL_ERROR,
};

static uint8_t expect_disconnect_prop_sei[] = {
	0xe0, 0x07, 0x00,
	/* Properties */
	0x05, ENCODED_PROP_SESSION_EXPIRY_INTERVAL,
};
static struct mqtt_disconnect_param disconnect_prop_sei = {
	.reason_code = MQTT_DISCONNECT_NORMAL,
	.prop.rx.has_session_expiry_interval = true,
	.prop.session_expiry_interval = TEST_PROP_SESSION_EXPIRY_INTERVAL,
};

static uint8_t expect_disconnect_prop_rs[] = {
	0xe0, 0x17, 0x82,
	/* Properties */
	0x15, ENCODED_PROP_REASON_STRING,
};
static struct mqtt_disconnect_param disconnect_prop_rs = {
	.reason_code = MQTT_DISCONNECT_PROTOCOL_ERROR,
	.prop.rx.has_reason_string = true,
	.prop.reason_string.utf8 = expect_disconnect_prop_rs + 7,
	.prop.reason_string.size = 18,
};

static uint8_t expect_disconnect_prop_up[] = {
	0xe0, 0x1a, 0x00,
	/* Properties */
	0x18, ENCODED_PROP_USER_PROPERTY,
};
static struct mqtt_disconnect_param disconnect_prop_up = {
	.reason_code = MQTT_DISCONNECT_NORMAL,
	.prop.rx.has_user_prop = true,
	.prop.user_prop[0].name.utf8 = expect_disconnect_prop_up + 7,
	.prop.user_prop[0].name.size = 9,
	.prop.user_prop[0].value.utf8 = expect_disconnect_prop_up + 18,
	.prop.user_prop[0].value.size = 10,
};

static uint8_t expect_disconnect_prop_sr[] = {
	0xe0, 0x13, 0x00,
	/* Properties */
	0x11, ENCODED_PROP_SERVER_REFERENCE,
};
static struct mqtt_disconnect_param disconnect_prop_sr = {
	.reason_code = MQTT_DISCONNECT_NORMAL,
	.prop.rx.has_server_reference = true,
	.prop.server_reference.utf8 = expect_disconnect_prop_sr + 7,
	.prop.server_reference.size = 14,
};

static struct mqtt_test disconnect_tests[] = {
	{ .test_name = "DISCONNECT, default",
	  .ctx = &disconnect_default, .test_fcn = test_msg_disconnect,
	  .expected = expect_disconnect_default,
	  .expected_len = sizeof(expect_disconnect_default) },
	{ .test_name = "DISCONNECT, error",
	  .ctx = &disconnect_error, .test_fcn = test_msg_disconnect,
	  .expected = expect_disconnect_error,
	  .expected_len = sizeof(expect_disconnect_error) },
	{ .test_name = "DISCONNECT, property Session Expiry Interval",
	  .ctx = &disconnect_prop_sei, .test_fcn = test_msg_disconnect,
	  .expected = expect_disconnect_prop_sei,
	  .expected_len = sizeof(expect_disconnect_prop_sei) },
	{ .test_name = "DISCONNECT, property Reason String",
	  .ctx = &disconnect_prop_rs, .test_fcn = test_msg_disconnect,
	  .expected = expect_disconnect_prop_rs,
	  .expected_len = sizeof(expect_disconnect_prop_rs) },
	{ .test_name = "DISCONNECT, property User Property",
	  .ctx = &disconnect_prop_up, .test_fcn = test_msg_disconnect,
	  .expected = expect_disconnect_prop_up,
	  .expected_len = sizeof(expect_disconnect_prop_up) },
	{ .test_name = "DISCONNECT, property Server Reference",
	  .ctx = &disconnect_prop_sr, .test_fcn = test_msg_disconnect,
	  .expected = expect_disconnect_prop_sr,
	  .expected_len = sizeof(expect_disconnect_prop_sr) },
};

ZTEST(mqtt_5_packet, test_mqtt_5_disconnect)
{
	run_packet_tests(disconnect_tests, ARRAY_SIZE(disconnect_tests));
}

static void test_msg_auth(struct mqtt_test *test)
{
	struct mqtt_auth_param *exp_param =
			(struct mqtt_auth_param *)test->ctx;
	struct mqtt_auth_param dec_param = { 0 };
	uint8_t type_and_flags;
	struct buf_ctx buf;
	uint32_t length;
	int ret;

	buf.cur = client.tx_buf;
	buf.end = client.tx_buf + client.tx_buf_size;

	ret = auth_encode(exp_param, &buf);
	zassert_ok(ret, "auth_encode failed");
	zassert_ok(validate_buffers(&buf, test->expected,
				    test->expected_len),
		   "Invalid packet content");

	buf.cur = test->expected;
	buf.end = test->expected + test->expected_len;

	ret = fixed_header_decode(&buf, &type_and_flags, &length);
	zassert_ok(ret, "fixed_header_decode failed");
	zassert_equal(type_and_flags & 0xF0, MQTT_PKT_TYPE_AUTH,
		      "Invalid packet type");
	zassert_equal(length, test->expected_len - 2,
		      "Invalid packet length");

	ret = auth_decode(&client, &buf, &dec_param);
	zassert_ok(ret, "auth_decode failed");
	zassert_ok(validate_structs(&dec_param, exp_param, sizeof(dec_param)),
		   "Incorrect AUTH params decoded");
}

static uint8_t expect_auth_default[] = {
	0xf0, 0x00,
};
static struct mqtt_auth_param auth_default = {
	.reason_code = MQTT_AUTH_SUCCESS,
};

static uint8_t expect_auth_reason[] = {
	0xf0, 0x01, 0x18,
};
static struct mqtt_auth_param auth_reason = {
	.reason_code = MQTT_AUTH_CONTINUE_AUTHENTICATION,
};

static uint8_t expect_auth_prop_am_ad[] = {
	0xf0, 0x1f, 0x00,
	/* Properties */
	0x1d, ENCODED_PROP_AUTHENTICATION_METHOD,
	ENCODED_PROP_AUTHENTICATION_DATA,
};
static struct mqtt_auth_param auth_prop_am_ad = {
	.reason_code = MQTT_AUTH_SUCCESS,
	.prop.rx.has_auth_method = true,
	.prop.auth_method.utf8 = expect_auth_prop_am_ad + 7,
	.prop.auth_method.size = 19,
	.prop.rx.has_auth_data = true,
	.prop.auth_data.data = expect_auth_prop_am_ad + 29,
	.prop.auth_data.len = 4,
};

static uint8_t expect_auth_prop_rs[] = {
	0xf0, 0x17, 0x18,
	/* Properties */
	0x15, ENCODED_PROP_REASON_STRING,
};
static struct mqtt_auth_param auth_prop_rs = {
	.reason_code = MQTT_AUTH_CONTINUE_AUTHENTICATION,
	.prop.rx.has_reason_string = true,
	.prop.reason_string.utf8 = expect_auth_prop_rs + 7,
	.prop.reason_string.size = 18,
};

static uint8_t expect_auth_prop_up[] = {
	0xf0, 0x1a, 0x00,
	/* Properties */
	0x18, ENCODED_PROP_USER_PROPERTY,
};
static struct mqtt_auth_param auth_prop_up = {
	.reason_code = MQTT_AUTH_SUCCESS,
	.prop.rx.has_user_prop = true,
	.prop.user_prop[0].name.utf8 = expect_auth_prop_up + 7,
	.prop.user_prop[0].name.size = 9,
	.prop.user_prop[0].value.utf8 = expect_auth_prop_up + 18,
	.prop.user_prop[0].value.size = 10,
};

static struct mqtt_test auth_tests[] = {
	{ .test_name = "AUTH, default",
	  .ctx = &auth_default, .test_fcn = test_msg_auth,
	  .expected = expect_auth_default,
	  .expected_len = sizeof(expect_auth_default) },
	{ .test_name = "AUTH, reason code",
	  .ctx = &auth_reason, .test_fcn = test_msg_auth,
	  .expected = expect_auth_reason,
	  .expected_len = sizeof(expect_auth_reason) },
	{ .test_name = "AUTH, property Authentication Method and Data",
	  .ctx = &auth_prop_am_ad, .test_fcn = test_msg_auth,
	  .expected = expect_auth_prop_am_ad,
	  .expected_len = sizeof(expect_auth_prop_am_ad) },
	{ .test_name = "AUTH, property Reason String",
	  .ctx = &auth_prop_rs, .test_fcn = test_msg_auth,
	  .expected = expect_auth_prop_rs,
	  .expected_len = sizeof(expect_auth_prop_rs) },
	{ .test_name = "AUTH, property User Property",
	  .ctx = &auth_prop_up, .test_fcn = test_msg_auth,
	  .expected = expect_auth_prop_up,
	  .expected_len = sizeof(expect_auth_prop_up) },
};

ZTEST(mqtt_5_packet, test_mqtt_5_auth)
{
	run_packet_tests(auth_tests, ARRAY_SIZE(auth_tests));
}

static void packet_tests_before(void *fixture)
{
	ARG_UNUSED(fixture);

	mqtt_client_init(&client);
	client.protocol_version = MQTT_VERSION_5_0;
	client.rx_buf = rx_buffer;
	client.rx_buf_size = sizeof(rx_buffer);
	client.tx_buf = tx_buffer;
	client.tx_buf_size = sizeof(tx_buffer);
}

static void packet_tests_after(void *fixture)
{
	ARG_UNUSED(fixture);

	memset(&client, 0, sizeof(client));
}

ZTEST_SUITE(mqtt_5_packet, NULL, NULL, packet_tests_before, packet_tests_after, NULL);
