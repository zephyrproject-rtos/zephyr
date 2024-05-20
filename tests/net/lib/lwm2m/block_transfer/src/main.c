/*
 * Copyright (c) 2023 Gardena GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "lwm2m_engine.h"
#include "lwm2m_object.h"

#include <zephyr/ztest.h>

/* Declaration of 'private' function */
int prepare_msg_for_send(struct lwm2m_message *msg);
int build_msg_block_for_send(struct lwm2m_message *msg, uint16_t block_num,
			     enum coap_block_size block_size);
int request_output_block_ctx(struct coap_block_context **ctx);
void release_output_block_ctx(struct coap_block_context ** const ctx);

BUILD_ASSERT(IS_ENABLED(CONFIG_LWM2M_COAP_BLOCK_TRANSFER),
	     "These tests expect to have block transfer enabled.");

#define EXPECTED_LWM2M_COAP_FULL_BUFFER_SIZE 256
BUILD_ASSERT(CONFIG_LWM2M_COAP_ENCODE_BUFFER_SIZE == EXPECTED_LWM2M_COAP_FULL_BUFFER_SIZE,
	     "The expected max message size is wrong.");

#define EXPECTED_NUM_OUTPUT_BLOCK_CONTEXT 3
BUILD_ASSERT(NUM_OUTPUT_BLOCK_CONTEXT == EXPECTED_NUM_OUTPUT_BLOCK_CONTEXT,
	     "The expected number of output block contexts is wrong.");

#define EXPECTED_DEFAULT_HEADER_OFFSET 4

struct net_block_transfer_fixture {
	uint8_t dummy_msg[CONFIG_LWM2M_COAP_ENCODE_BUFFER_SIZE];
	struct lwm2m_ctx ctx;
	struct lwm2m_message msg;
};

static void *net_block_transfer_setup(void)
{
	static struct net_block_transfer_fixture f;

	for (uint_fast16_t i = 0; i < ARRAY_SIZE(f.dummy_msg); ++i) {
		f.dummy_msg[i] = (uint8_t)i;
	}

	return &f;
}

static void net_block_transfer_before(void *f)
{
	struct net_block_transfer_fixture *fixture = (struct net_block_transfer_fixture *)f;

	memset(&fixture->ctx, 0, sizeof(struct lwm2m_ctx));
	memset(&fixture->msg, 0, sizeof(struct lwm2m_message));
	fixture->msg.ctx = &fixture->ctx;
}

static void net_block_transfer_after(void *f)
{
	struct net_block_transfer_fixture *fixture = (struct net_block_transfer_fixture *)f;

	lwm2m_reset_message(&fixture->msg, true);
}

ZTEST_F(net_block_transfer, test_init_message_use_big_buffer)
{
	int ret;
	struct lwm2m_message *msg = &fixture->msg;


	ret = lwm2m_init_message(msg);
	zassert_ok(ret, "Failed to initialize lwm2m message");

	zassert_not_equal(msg->msg_data, msg->cpkt.data,
			  "Default data buffer should not be used for writing body");
	zassert_equal(msg->cpkt.data, msg->body_encode_buffer.data,
			  "Full body buffer should be in use");

	zassert_equal(EXPECTED_LWM2M_COAP_FULL_BUFFER_SIZE, msg->cpkt.max_len,
		      "Max length for the package is wrong");

	zassert_equal(EXPECTED_DEFAULT_HEADER_OFFSET, msg->cpkt.offset);

	/* write to buffer in a similar way as the writers */
	msg->out.out_cpkt = &msg->cpkt;
	ret = buf_append(CPKT_BUF_WRITE(msg->out.out_cpkt), fixture->dummy_msg,
			 EXPECTED_LWM2M_COAP_FULL_BUFFER_SIZE - EXPECTED_DEFAULT_HEADER_OFFSET);
	zassert_ok(ret, "Should be able to write to buffer");
	zassert_equal(msg->cpkt.max_len, msg->cpkt.offset, "Buffer should be full");

	const uint8_t one_byte = 0xAB;

	ret = buf_append(CPKT_BUF_WRITE(msg->out.out_cpkt), &one_byte, 1);
	zassert_equal(ret, -ENOMEM, "Should not be able to write to full buffer");
}

#define EXPECTED_HEADERS_LEN 7
ZTEST_F(net_block_transfer, test_one_block_with_big_buffer)
{
	int ret;
	struct lwm2m_message *msg = &fixture->msg;

	/*  Arrange */
	ret = lwm2m_init_message(msg);
	zassert_equal(0, ret, "Failed to initialize lwm2m message");

	zassert_not_equal(msg->msg_data, msg->cpkt.data,
			  "Big body data buffer should be used for writing body");
	zassert_equal(msg->cpkt.data, msg->body_encode_buffer.data,
		      "Full body buffer should be in use");

	ret = coap_append_option_int(&msg->cpkt, COAP_OPTION_CONTENT_FORMAT,
				     COAP_CONTENT_FORMAT_APP_LINK_FORMAT);
	zassert_equal(0, ret, "Not able to append option");

	ret = coap_packet_append_payload_marker(&msg->cpkt);
	zassert_equal(0, ret, "Not able to append payload marker");

	ret = buf_append(CPKT_BUF_WRITE(&msg->cpkt), fixture->dummy_msg,
			 CONFIG_LWM2M_COAP_BLOCK_SIZE);
	zassert_ok(ret, "Should be able to write to buffer");

	uint16_t payload_len;

	coap_packet_get_payload(&msg->cpkt, &payload_len);
	zassert_equal(payload_len, CONFIG_LWM2M_COAP_BLOCK_SIZE,
		      "Block was not filled as expected");

	/*  Act */
	ret = prepare_msg_for_send(msg);
	zassert_equal(0, ret, "Preparing message for sending failed");

	/*  Assert */
	zassert_equal(msg->msg_data, msg->cpkt.data,
		      "Default data buffer should be used for sending the block");
	zassert_is_null(msg->body_encode_buffer.data, "Complete body buffer should not be set");

	const uint8_t *payload = coap_packet_get_payload(&msg->cpkt, &payload_len);

	zassert_not_null(payload, "Payload expected");
	zassert_equal(payload_len, CONFIG_LWM2M_COAP_BLOCK_SIZE,
		      "Block was not filled as expected");

	zassert_equal(EXPECTED_HEADERS_LEN, msg->cpkt.hdr_len + msg->cpkt.opt_len + 1,
		      "Headers length not as expected");
	/* payload should start after headers, options and payload marker (size: 1) */
	zassert_equal(payload, msg->cpkt.data + EXPECTED_HEADERS_LEN,
		      "Payload not starting at expected address");

	const uint8_t expected_headers[EXPECTED_HEADERS_LEN] = {0x40, 0, 0, 0, 0xc1, 0x28, 0xff};

	zassert_mem_equal(msg->cpkt.data, expected_headers, EXPECTED_HEADERS_LEN,
			  "Payload not starting at expected address");

	/* check payload */
	for (uint_fast8_t i = 0; i < payload_len; ++i) {
		zassert_equal(payload[i], i, "Byte %i in payload is wrong", i);
	}

	/* check first byte after payload */
	zassert_equal(payload[payload_len], 0x00, "Byte after payload is wrong");
}

ZTEST_F(net_block_transfer, test_build_first_block_for_send)
{
	int ret;
	struct lwm2m_message *msg = &fixture->msg;
	uint16_t payload_len;

	/*  Arrange */
	msg->code = COAP_METHOD_GET;
	ret = lwm2m_init_message(msg);
	zassert_equal(0, ret, "Failed to initialize lwm2m message");

	ret = coap_append_option_int(&msg->cpkt, COAP_OPTION_CONTENT_FORMAT,
				     COAP_CONTENT_FORMAT_APP_LINK_FORMAT);
	zassert_equal(0, ret, "Not able to append option");

	ret = coap_append_option_int(&msg->cpkt, COAP_OPTION_ACCEPT, COAP_CONTENT_FORMAT_APP_JSON);
	zassert_equal(0, ret, "Not able to append option");

	const uint8_t expected_header_len = 4;

	zassert_equal(msg->cpkt.hdr_len, expected_header_len, "Header length not as expected");

	const uint8_t expected_options_len = 4;

	zassert_equal(msg->cpkt.opt_len, expected_options_len, "Options length not as expected");

	ret = coap_packet_append_payload_marker(&msg->cpkt);
	zassert_equal(0, ret, "Not able to append payload marker");

	ret = buf_append(CPKT_BUF_WRITE(&msg->cpkt), fixture->dummy_msg,
			 2 * CONFIG_LWM2M_COAP_BLOCK_SIZE);
	zassert_ok(ret, "Should be able to write to buffer");

	zassert_not_equal(msg->msg_data, msg->cpkt.data, "Buffer for block data is not yet in use");

	/*  Act */
	ret = prepare_msg_for_send(msg);
	zassert_equal(ret, 0, "Could not create first block");

	/*  Assert */
	zassert_equal(msg->msg_data, msg->cpkt.data, "Buffer for block data is not in use");

	ret = coap_get_option_int(&msg->cpkt, COAP_OPTION_BLOCK1);
	zassert(ret > 0, "block 1 option not set");

	const uint8_t *payload = coap_packet_get_payload(&msg->cpkt, &payload_len);

	zassert_not_null(payload, "Payload expected");
	zassert_equal(payload_len, CONFIG_LWM2M_COAP_BLOCK_SIZE, "Wrong payload size");
	zassert_equal(0x00, payload[0], "First byte in payload wrong");
	zassert_equal(0x3f, payload[CONFIG_LWM2M_COAP_BLOCK_SIZE - 1],
		      "Last byte in payload wrong");
}

ZTEST_F(net_block_transfer, test_build_blocks_for_send_exactly_2_blocks)
{
	int ret;
	struct lwm2m_message *msg = &fixture->msg;
	uint16_t payload_len;
	const uint8_t *payload;

	/*  Arrange */
	msg->code = COAP_METHOD_PUT;
	ret = lwm2m_init_message(msg);
	zassert_equal(0, ret, "Failed to initialize lwm2m message");

	const uint8_t *query = "query";

	ret = coap_packet_append_option(&msg->cpkt, COAP_OPTION_URI_QUERY, query, strlen(query));
	zassert_equal(0, ret, "Not able to append option");

	ret = coap_append_option_int(&msg->cpkt, COAP_OPTION_ACCEPT,
				     COAP_CONTENT_FORMAT_TEXT_PLAIN);
	zassert_equal(0, ret, "Not able to append option");

	const uint8_t expected_header_len = 4;

	zassert_equal(msg->cpkt.hdr_len, expected_header_len, "Header length not as expected");

	const uint8_t expected_options_len = 8;

	zassert_equal(msg->cpkt.opt_len, expected_options_len, "Options length not as expected");

	ret = coap_packet_append_payload_marker(&msg->cpkt);
	zassert_equal(0, ret, "Not able to append payload marker");

	ret = buf_append(CPKT_BUF_WRITE(&msg->cpkt), fixture->dummy_msg,
			 2 * CONFIG_LWM2M_COAP_BLOCK_SIZE);
	zassert_ok(ret, "Should be able to write to buffer");

	zassert_not_equal(msg->msg_data, msg->cpkt.data, "Buffer for block data is not yet in use");

	/*  block 0 */
	ret = prepare_msg_for_send(msg);
	zassert_equal(ret, 0, "Could not create first block");

	zassert_equal(msg->msg_data, msg->cpkt.data, "Buffer for block data is not in use");

	ret = coap_get_option_int(&msg->cpkt, COAP_OPTION_BLOCK1);
	zassert(ret > 0, "block 1 option not set");

	payload = coap_packet_get_payload(&msg->cpkt, &payload_len);

	zassert_not_null(payload, "Payload expected");
	zassert_equal(payload_len, CONFIG_LWM2M_COAP_BLOCK_SIZE, "Wrong payload size");
	zassert_equal(0x00, payload[0], "First byte in payload wrong");
	zassert_equal(0x3f, payload[CONFIG_LWM2M_COAP_BLOCK_SIZE - 1],
		      "Last byte in payload wrong");

	/* block 1 */
	ret = build_msg_block_for_send(msg, 1, COAP_BLOCK_64);
	zassert_equal(ret, 0, "Could not create second block");

	ret = coap_get_option_int(&msg->cpkt, COAP_OPTION_BLOCK1);
	zassert(ret > 0, "block 1 option not set");

	payload = coap_packet_get_payload(&msg->cpkt, &payload_len);

	zassert_not_null(payload, "Payload expected");
	zassert_equal(payload_len, CONFIG_LWM2M_COAP_BLOCK_SIZE, "Wrong payload size");
	zassert_equal(0x40, payload[0], "First byte in payload wrong");
	zassert_equal(0x7f, payload[CONFIG_LWM2M_COAP_BLOCK_SIZE - 1],
		      "Last byte in payload wrong");

	/* block 2 doesn't exist */
	ret = build_msg_block_for_send(msg, 2, COAP_BLOCK_64);
	zassert_equal(ret, -EINVAL, "Could not create second block");
}

ZTEST_F(net_block_transfer, test_build_blocks_for_send_more_than_2_blocks)
{
	int ret;
	struct lwm2m_message *msg = &fixture->msg;
	uint16_t payload_len;
	const uint8_t *payload;

	/*  Arrange */
	msg->code = COAP_METHOD_DELETE;
	ret = lwm2m_init_message(msg);
	zassert_equal(0, ret, "Failed to initialize lwm2m message");

	const uint8_t *proxy_scheme = "coap";

	ret = coap_packet_append_option(&msg->cpkt, COAP_OPTION_PROXY_SCHEME, proxy_scheme,
					strlen(proxy_scheme));
	zassert_equal(0, ret, "Not able to append option");

	ret = coap_append_option_int(&msg->cpkt, COAP_OPTION_CONTENT_FORMAT,
				     COAP_CONTENT_FORMAT_APP_JSON);
	zassert_equal(0, ret, "Not able to append option");

	const uint8_t expected_header_len = 4;

	zassert_equal(msg->cpkt.hdr_len, expected_header_len, "Header length not as expected");

	const uint8_t expected_options_len = 8;

	zassert_equal(msg->cpkt.opt_len, expected_options_len, "Options length not as expected");

	ret = coap_packet_append_payload_marker(&msg->cpkt);
	zassert_equal(0, ret, "Not able to append payload marker");

	ret = buf_append(CPKT_BUF_WRITE(&msg->cpkt), fixture->dummy_msg,
			 2 * CONFIG_LWM2M_COAP_BLOCK_SIZE + 1);
	zassert_ok(ret, "Should be able to write to buffer");

	zassert_not_equal(msg->msg_data, msg->cpkt.data, "Buffer for block data is not yet in use");

	/*  block 0 */
	ret = prepare_msg_for_send(msg);
	zassert_equal(ret, 0, "Could not create first block");

	zassert_equal(msg->msg_data, msg->cpkt.data, "Buffer for block data is not in use");

	ret = coap_get_option_int(&msg->cpkt, COAP_OPTION_BLOCK1);
	zassert(ret > 0, "block 1 option not set");

	payload = coap_packet_get_payload(&msg->cpkt, &payload_len);

	zassert_not_null(payload, "Payload expected");
	zassert_equal(payload_len, CONFIG_LWM2M_COAP_BLOCK_SIZE, "Wrong payload size");
	zassert_equal(0x00, payload[0], "First byte in payload wrong");
	zassert_equal(0x3f, payload[CONFIG_LWM2M_COAP_BLOCK_SIZE - 1],
		      "Last byte in payload wrong");

	/* block 1 */
	ret = build_msg_block_for_send(msg, 1, COAP_BLOCK_64);
	zassert_equal(ret, 0, "Could not create second block");

	ret = coap_get_option_int(&msg->cpkt, COAP_OPTION_BLOCK1);
	zassert(ret > 0, "block 1 option not set");

	payload = coap_packet_get_payload(&msg->cpkt, &payload_len);

	zassert_not_null(payload, "Payload expected");
	zassert_equal(payload_len, CONFIG_LWM2M_COAP_BLOCK_SIZE, "Wrong payload size");
	zassert_equal(0x40, payload[0], "First byte in payload wrong");
	zassert_equal(0x7f, payload[CONFIG_LWM2M_COAP_BLOCK_SIZE - 1],
		      "Last byte in payload wrong");

	/* block 2 */
	ret = build_msg_block_for_send(msg, 2, COAP_BLOCK_64);
	zassert_equal(ret, 0, "Could not create second block");

	ret = coap_get_option_int(&msg->cpkt, COAP_OPTION_BLOCK1);
	zassert(ret > 0, "block 1 option not set");

	payload = coap_packet_get_payload(&msg->cpkt, &payload_len);

	zassert_not_null(payload, "Payload expected");
	zassert_equal(payload_len, 1, "Wrong payload size");
	zassert_equal(0x80, payload[0], "First (and only) byte in payload wrong");

	/* block 3 doesn't exist */
	ret = build_msg_block_for_send(msg, 3, COAP_BLOCK_64);
	zassert_equal(ret, -EINVAL, "Could not create second block");
}

ZTEST_F(net_block_transfer, test_block_context)
{
	struct coap_block_context *ctx0, *ctx1, *ctx2, *ctx3, *ctx4;
	int ret;

	zassert_equal(NUM_OUTPUT_BLOCK_CONTEXT, 3);

	/* block context 0 */
	ret = request_output_block_ctx(&ctx0);
	zassert_ok(ret);
	zassert_not_null(ctx0);
	/* block context 1 */
	ret = request_output_block_ctx(&ctx1);
	zassert_ok(ret);
	zassert_not_null(ctx1);
	/* block context 2 */
	ret = request_output_block_ctx(&ctx2);
	zassert_ok(ret);
	zassert_not_null(ctx2);

	/* Get one context more than available */
	ret = request_output_block_ctx(&ctx3);
	zassert_equal(ret, -ENOMEM);
	zassert_is_null(ctx3);

	/* release one block context */
	release_output_block_ctx(&ctx2);
	zassert_is_null(ctx2);

	/* get another block context */
	ret = request_output_block_ctx(&ctx4);
	zassert_ok(ret);
	zassert_not_null(ctx4);

	/* release all block contexts */
	release_output_block_ctx(&ctx0);
	zassert_is_null(ctx0);
	release_output_block_ctx(&ctx1);
	zassert_is_null(ctx1);
	release_output_block_ctx(&ctx2);
	zassert_is_null(ctx2);
	release_output_block_ctx(&ctx3);
	zassert_is_null(ctx3);
	release_output_block_ctx(&ctx4);
	zassert_is_null(ctx4);
}

ZTEST_SUITE(net_block_transfer, NULL, net_block_transfer_setup, net_block_transfer_before,
	    net_block_transfer_after, NULL);
