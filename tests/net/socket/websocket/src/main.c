/*
 * Copyright (c) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <logging/log.h>
LOG_MODULE_REGISTER(net_test, CONFIG_NET_WEBSOCKET_LOG_LEVEL);

#include <ztest_assert.h>

#include <net/net_ip.h>
#include <net/socket.h>
#include <net/websocket.h>

#include "websocket_internal.h"

#define MAX_RECV_BUF_LEN 256
static u8_t recv_buf[MAX_RECV_BUF_LEN];

/* We need to allocate bigger buffer for the websocket data we receive so that
 * the websocket header fits into it.
 */
#define EXTRA_BUF_SPACE 30

static u8_t temp_recv_buf[MAX_RECV_BUF_LEN + EXTRA_BUF_SPACE];
static u8_t feed_buf[MAX_RECV_BUF_LEN + EXTRA_BUF_SPACE];

struct test_data {
	u8_t *input_buf;
	size_t input_len;
	struct websocket_context *ctx;
};

static int test_recv_buf(u8_t *feed_buf, size_t feed_len,
			 struct websocket_context *ctx,
			 u32_t *msg_type, u64_t *remaining,
			 u8_t *recv_buf, size_t recv_len)
{
	static struct test_data test_data;
	int ctx_ptr;

	test_data.ctx = ctx;
	test_data.input_buf = feed_buf;
	test_data.input_len = feed_len;

	ctx_ptr = POINTER_TO_INT(&test_data);

	return websocket_recv_msg(ctx_ptr, recv_buf, recv_len,
				  msg_type, remaining, K_NO_WAIT);
}

/* Websocket frame, header is 6 bytes, FIN bit is set, opcode is text (1),
 * payload length is 12, masking key is e17e8eb9,
 * unmasked data is "test message"
 */
static const unsigned char frame1[] = {
	0x81, 0x8c, 0xe1, 0x7e, 0x8e, 0xb9, 0x95, 0x1b,
	0xfd, 0xcd, 0xc1, 0x13, 0xeb, 0xca, 0x92, 0x1f,
	0xe9, 0xdc
};

static const unsigned char frame1_msg[] = {
	/* Null added for printing purposes */
	't', 'e', 's', 't', ' ', 'm', 'e', 's', 's', 'a', 'g', 'e', '\0'
};

/* The frame2 has frame1 + frame1. The idea is to test a case where we
 * read full frame1 and then part of second frame
 */
static const unsigned char frame2[] = {
	0x81, 0x8c, 0xe1, 0x7e, 0x8e, 0xb9, 0x95, 0x1b,
	0xfd, 0xcd, 0xc1, 0x13, 0xeb, 0xca, 0x92, 0x1f,
	0xe9, 0xdc,
	0x81, 0x8c, 0xe1, 0x7e, 0x8e, 0xb9, 0x95, 0x1b,
	0xfd, 0xcd, 0xc1, 0x13, 0xeb, 0xca, 0x92, 0x1f,
	0xe9, 0xdc
};

#define FRAME1_HDR_SIZE (sizeof(frame1) - (sizeof(frame1_msg) - 1))

static void test_recv(int count)
{
	struct websocket_context ctx;
	u32_t msg_type = -1;
	u64_t remaining = -1;
	int total_read = 0;
	int ret, i, left;

	memset(&ctx, 0, sizeof(ctx));

	ctx.tmp_buf = temp_recv_buf;
	ctx.tmp_buf_len = sizeof(temp_recv_buf);
	ctx.tmp_buf_pos = 0;

	memcpy(feed_buf, &frame1, sizeof(frame1));

	NET_DBG("Reading %d bytes at a time, frame %zd hdr %zd", count,
		sizeof(frame1), FRAME1_HDR_SIZE);

	/* We feed the frame N byte(s) at a time */
	for (i = 0; i < sizeof(frame1) / count; i++) {
		ret = test_recv_buf(&feed_buf[i * count], count,
				    &ctx, &msg_type, &remaining,
				    recv_buf + total_read,
				    sizeof(recv_buf) - total_read);
		if (count < 7 && (i * count) < FRAME1_HDR_SIZE) {
			zassert_equal(ret, -EAGAIN,
				      "[%d] Header parse failed (ret %d)",
				      i * count, ret);
		} else {
			total_read += ret;
		}
	}

	/* Read any remaining data */
	left = sizeof(frame1) % count;
	if (left > 0) {
		/* Some leftover bytes are still there */
		ret = test_recv_buf(&feed_buf[sizeof(frame1) - left], left,
				    &ctx, &msg_type, &remaining,
				    recv_buf + total_read,
				    sizeof(recv_buf) - total_read);
		total_read += ret;
		zassert_equal(total_read, sizeof(frame1) - FRAME1_HDR_SIZE,
			      "Invalid amount of data read (%d)", ret);

	} else if (total_read < (sizeof(frame1) - FRAME1_HDR_SIZE)) {
		/* We read the whole message earlier, but we have parsed
		 * only part of the message. Parse the reset of the message
		 * here.
		 */
		ret = test_recv_buf(&feed_buf[FRAME1_HDR_SIZE + total_read],
			    sizeof(frame1) - FRAME1_HDR_SIZE - total_read,
				    &ctx, &msg_type, &remaining,
				    recv_buf + total_read,
				    sizeof(recv_buf) - total_read);
		total_read += ret;
		zassert_equal(total_read, sizeof(frame1) - FRAME1_HDR_SIZE,
			      "Invalid amount of data read (%d)", ret);
	}

	zassert_mem_equal(recv_buf, frame1_msg, sizeof(frame1_msg) - 1,
			  "Invalid message, should be '%s' was '%s'",
			  frame1_msg, recv_buf);

	zassert_equal(remaining, 0, "Msg not empty");
}

static void test_recv_1_byte(void)
{
	test_recv(1);
}

static void test_recv_2_byte(void)
{
	test_recv(2);
}

static void test_recv_3_byte(void)
{
	test_recv(3);
}

static void test_recv_6_byte(void)
{
	test_recv(6);
}

static void test_recv_7_byte(void)
{
	test_recv(7);
}

static void test_recv_8_byte(void)
{
	test_recv(8);
}

static void test_recv_9_byte(void)
{
	test_recv(9);
}

static void test_recv_10_byte(void)
{
	test_recv(10);
}

static void test_recv_12_byte(void)
{
	test_recv(12);
}

static void test_recv_whole_msg(void)
{
	test_recv(sizeof(frame1));
}

static void test_recv_2(int count)
{
	struct websocket_context ctx;
	u32_t msg_type = -1;
	u64_t remaining = -1;
	int total_read = 0;
	int ret;

	memset(&ctx, 0, sizeof(ctx));

	ctx.tmp_buf = temp_recv_buf;
	ctx.tmp_buf_len = sizeof(temp_recv_buf);

	memcpy(feed_buf, &frame2, sizeof(frame2));

	NET_DBG("Reading %d bytes at a time, frame %zd hdr %zd", count,
		sizeof(frame2), FRAME1_HDR_SIZE);

	total_read = test_recv_buf(&feed_buf[0], count, &ctx, &msg_type,
				   &remaining, recv_buf, sizeof(recv_buf));

	zassert_mem_equal(recv_buf, frame1_msg, sizeof(frame1_msg) - 1,
			  "Invalid message, should be '%s' was '%s'",
			  frame1_msg, recv_buf);

	zassert_equal(remaining, 0, "Msg not empty");

	/* Then read again, now we should get EAGAIN as the second message
	 * header is partially read.
	 */
	ret = test_recv_buf(&feed_buf[sizeof(frame1)], count, &ctx, &msg_type,
			    &remaining, recv_buf, sizeof(recv_buf));

	zassert_equal(ret, sizeof(frame1_msg) - 1,
		      "2nd header parse failed (ret %d)", ret);

	zassert_equal(remaining, 0, "Msg not empty");
}

static void test_recv_two_msg(void)
{
	test_recv_2(sizeof(frame1) + FRAME1_HDR_SIZE / 2);
}

void test_main(void)
{
	k_thread_system_pool_assign(k_current_get());

	ztest_test_suite(websocket,
			 ztest_unit_test(test_recv_1_byte),
			 ztest_unit_test(test_recv_2_byte),
			 ztest_unit_test(test_recv_3_byte),
			 ztest_unit_test(test_recv_6_byte),
			 ztest_unit_test(test_recv_7_byte),
			 ztest_unit_test(test_recv_8_byte),
			 ztest_unit_test(test_recv_9_byte),
			 ztest_unit_test(test_recv_10_byte),
			 ztest_unit_test(test_recv_12_byte),
			 ztest_unit_test(test_recv_whole_msg),
			 ztest_unit_test(test_recv_two_msg)
		);

	ztest_run_test_suite(websocket);
}
