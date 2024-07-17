/*
 * Copyright (c) 2023, Emna Rekik
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "server_internal.h"

#include <string.h>

#include <zephyr/net/http/service.h>
#include <zephyr/net/socket.h>
#include <zephyr/posix/sys/eventfd.h>
#include <zephyr/ztest.h>

#define SUPPORT_BACKWARD_COMPATIBILITY 1
#define SUPPORT_HTTP_SERVER_UPGRADE    2
#define BUFFER_SIZE                    512
#define SERVER_IPV4_ADDR               "127.0.0.1"
#define SERVER_PORT                    8080
#define TIMEOUT_S                      1

/* Magic, SETTINGS[0], HEADERS[1]: GET /, HEADERS[3]: GET /index.html, SETTINGS[0], GOAWAY[0]*/
static const unsigned char request_get_2_streams[] = {
	/* Magic */
	0x50, 0x52, 0x49, 0x20, 0x2a, 0x20, 0x48, 0x54, 0x54, 0x50, 0x2f, 0x32,
	0x2e, 0x30, 0x0d, 0x0a, 0x0d, 0x0a, 0x53, 0x4d, 0x0d, 0x0a, 0x0d, 0x0a,
	/* SETTINGS[0] */
	0x00, 0x00, 0x0c, 0x04, 0x00, 0x00, 0x00, 0x00,	0x00,
	0x00, 0x03, 0x00, 0x00, 0x00, 0x64, 0x00, 0x04, 0x00, 0x00, 0xff, 0xff,
	/* HEADERS[1]: GET / */
	0x00, 0x00, 0x21, 0x01, 0x05, 0x00, 0x00, 0x00, 0x01,
	0x82, 0x84, 0x86, 0x41, 0x8a, 0x0b, 0xe2, 0x5c, 0x0b, 0x89, 0x70, 0xdc,
	0x78, 0x0f, 0x03, 0x53, 0x03, 0x2a, 0x2f, 0x2a, 0x90, 0x7a, 0x8a, 0xaa,
	0x69, 0xd2, 0x9a, 0xc4, 0xc0, 0x57, 0x68, 0x0b, 0x83,
	/* HEADERS[3]: GET /index.html */
	0x00, 0x00, 0x21, 0x01, 0x05, 0x00, 0x00, 0x00, 0x03,
	0x82, 0x85, 0x86, 0x41, 0x8a, 0x0b, 0xe2, 0x5c, 0x0b, 0x89, 0x70, 0xdc,
	0x78, 0x0f, 0x03, 0x53, 0x03, 0x2a, 0x2f, 0x2a, 0x90, 0x7a, 0x8a, 0xaa,
	0x69, 0xd2, 0x9a, 0xc4, 0xc0, 0x57, 0x68, 0x0b, 0x83,
	/* SETTINGS[0] */
	0x00, 0x00, 0x00, 0x04, 0x01, 0x00, 0x00, 0x00, 0x00,
	/*  GOAWAY[0] */
	0x00, 0x00, 0x08, 0x07, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};

static uint16_t test_http_service_port = SERVER_PORT;
HTTP_SERVICE_DEFINE(test_http_service, SERVER_IPV4_ADDR,
		    &test_http_service_port, 1, 10, NULL);

static const char static_resource_payload[] = "Hello, World!";
struct http_resource_detail_static static_resource_detail = {
	.common = {
			.type = HTTP_RESOURCE_TYPE_STATIC,
			.bitmask_of_supported_http_methods = BIT(HTTP_GET),
		},
	.static_data = static_resource_payload,
	.static_data_len = sizeof(static_resource_payload),
};

HTTP_RESOURCE_DEFINE(static_resource, test_http_service, "/",
		     &static_resource_detail);

static int client_fd = -1;

static void test_streams(void)
{
	int ret;
	static unsigned char buf[BUFFER_SIZE];
	unsigned int length;
	uint8_t type;
	size_t offset;
	uint32_t stream_id;

	ret = zsock_send(client_fd, request_get_2_streams,
			 sizeof(request_get_2_streams), 0);
	zassert_not_equal(ret, -1, "send() failed (%d)", errno);

	memset(buf, 0, sizeof(buf));
	offset = 0;
	do {
		ret = zsock_recv(client_fd, buf + offset, sizeof(buf) - offset, 0);
		zassert_not_equal(ret, -1, "recv() failed (%d)", errno);

		offset += ret;
	} while (ret > 0);

	/* Settings frame is expected twice (server settings + settings ACK) */
	length = (buf[0] << 16) | (buf[1] << 8) | buf[2];
	length += 9;
	type = buf[3];
	stream_id = (buf[5] << 24) | (buf[6] << 16) | (buf[7] << 8) | buf[8];
	stream_id &= 0x7fffffff;

	zassert_true((type == 0x4 && stream_id == 0),
		     "Expected a SETTINGS frame with stream ID 0");
	zassert_true(offset > length, "Parsing error, buffer exceeded");

	offset -= length;
	memmove(buf, buf + length, offset);

	length = (buf[0] << 16) | (buf[1] << 8) | buf[2];
	length += 9;
	type = buf[3];
	stream_id = (buf[5] << 24) | (buf[6] << 16) | (buf[7] << 8) | buf[8];
	stream_id &= 0x7fffffff;

	zassert_true((type == 0x4 && stream_id == 0),
		     "Expected a SETTINGS frame with stream ID 0");
	zassert_true(offset > length, "Parsing error, buffer exceeded");

	offset -= length;
	memmove(buf, buf + length, offset);

	length = (buf[0] << 16) | (buf[1] << 8) | buf[2];
	length += 9;
	type = buf[3];
	stream_id = (buf[5] << 24) | (buf[6] << 16) | (buf[7] << 8) | buf[8];
	stream_id &= 0x7fffffff;

	zassert_true((type == 0x1 && stream_id == 1),
		     "Expected a HEADERS frame with stream ID 1, got %d", stream_id);
	zassert_true(offset > length, "Parsing error, buffer exceeded");

	offset -= length;
	memmove(buf, buf + length, offset);

	length = (buf[0] << 16) | (buf[1] << 8) | buf[2];
	length += 9;
	type = buf[3];
	stream_id = (buf[5] << 24) | (buf[6] << 16) | (buf[7] << 8) | buf[8];
	stream_id &= 0x7fffffff;
	buf[9] = 0;

	zassert_true((type == 0x0 && stream_id == 1),
		     "Expected a DATA frame with stream ID 1, got %d", stream_id);
	zassert_true(offset > length, "Parsing error, buffer exceeded");

	offset -= length;
	memmove(buf, buf + length, offset);

	length = (buf[0] << 16) | (buf[1] << 8) | buf[2];
	length += 9;
	type = buf[3];
	stream_id = (buf[5] << 24) | (buf[6] << 16) | (buf[7] << 8) | buf[8];
	stream_id &= 0x7fffffff;

	zassert_true((type == 0x1 && stream_id == 3),
		     "Expected a HEADERS frame with stream ID 3");
	zassert_true(offset >= length, "Parsing error, buffer exceeded");

	offset -= length;
	memmove(buf, buf + length, offset);

	length = (buf[0] << 16) | (buf[1] << 8) | buf[2];
	length += 9;
	type = buf[3];
	stream_id = (buf[5] << 24) | (buf[6] << 16) | (buf[7] << 8) | buf[8];
	stream_id &= 0x7fffffff;

	zassert_true((type == 0x0 && stream_id == 3),
		     "Expected a DATA frame with stream ID 3");
}

ZTEST(server_function_tests, test_http_concurrent_streams)
{
	test_streams();
}

static void test_common(int test_support)
{
	int ret;
	static unsigned char buf[BUFFER_SIZE];

	if (test_support == SUPPORT_BACKWARD_COMPATIBILITY) {

		char *http1_request = "GET / HTTP/1.1\r\n"
				      "Host: 127.0.0.1:8080\r\n"
				      "User-Agent: curl/7.68.0\r\n"
				      "Accept: */*\r\n"
				      "Accept-Encoding: deflate, gzip, br\r\n"
				      "\r\n";

		ret = zsock_send(client_fd, http1_request, strlen(http1_request), 0);
		zassert_not_equal(ret, -1, "send() failed (%d)", errno);

		char expected_response[] = "HTTP/1.1 200 OK\r\n"
					   "Content-Type: text/html\r\n"
					   "Content-Length: 14\r\n"
					   "\r\n";

		memset(buf, 0, sizeof(buf));
		ret = zsock_recv(client_fd, buf, sizeof(buf), 0);
		zassert_not_equal(ret, -1, "recv() failed (%d)", errno);

		zassert_equal(strncmp(buf, expected_response,
				      strlen(expected_response)), 0,
			      "Received data doesn't match expected response");

	} else if (test_support == SUPPORT_HTTP_SERVER_UPGRADE) {

		ret = zsock_send(client_fd, request_get_2_streams,
				 sizeof(request_get_2_streams), 0);
		zassert_not_equal(ret, -1, "send() failed (%d)", errno);

		memset(buf, 0, sizeof(buf));
		ret = zsock_recv(client_fd, buf, sizeof(buf), 0);
		zassert_not_equal(ret, -1, "recv() failed (%d)", errno);

		uint8_t type = buf[3];

		zassert_true(type == 0x4, "Expected a SETTINGS frame");
	}
}

ZTEST(server_function_tests, test_http_upgrade)
{
	test_common(SUPPORT_HTTP_SERVER_UPGRADE);
}

ZTEST(server_function_tests, test_backward_compatibility)
{
	test_common(SUPPORT_BACKWARD_COMPATIBILITY);
}

ZTEST(server_function_tests_no_init, test_http_server_start_stop)
{
	struct sockaddr_in sa = { 0 };
	int ret;

	sa.sin_family = AF_INET;
	sa.sin_port = htons(SERVER_PORT);

	ret = zsock_inet_pton(AF_INET, SERVER_IPV4_ADDR, &sa.sin_addr.s_addr);
	zassert_equal(1, ret, "inet_pton() failed to convert %s", SERVER_IPV4_ADDR);

	zassert_ok(http_server_start(), "Failed to start the server");
	zassert_not_ok(http_server_start(), "Server start should report na error.");

	zassert_ok(http_server_stop(), "Failed to stop the server");
	zassert_not_ok(http_server_stop(), "Server stop should report na error.");

	zassert_ok(http_server_start(), "Failed to start the server");

	/* Server should be listening now. */
	ret = zsock_socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	zassert_not_equal(ret, -1, "failed to create client socket (%d)", errno);
	client_fd = ret;

	zassert_ok(zsock_connect(client_fd, (struct sockaddr *)&sa, sizeof(sa)),
		   "failed to connect to the server (%d)", errno);
	zassert_ok(zsock_close(client_fd), "close() failed on the client fd (%d)", errno);
	client_fd = -1;

	/* Check if the server can be restarted again after client connected. */
	zassert_ok(http_server_stop(), "Failed to stop the server");
	zassert_ok(http_server_start(), "Failed to start the server");

	/* Let the server thread run. */
	k_msleep(CONFIG_HTTP_SERVER_RESTART_DELAY + 10);

	ret = zsock_socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	zassert_not_equal(ret, -1, "failed to create client socket (%d)", errno);
	client_fd = ret;

	zassert_ok(zsock_connect(client_fd, (struct sockaddr *)&sa, sizeof(sa)),
		   "failed to connect to the server (%d)", errno);
	zassert_ok(zsock_close(client_fd), "close() failed on the client fd (%d)", errno);
	client_fd = -1;

	zassert_ok(http_server_stop(), "Failed to stop the server");
}

ZTEST(server_function_tests_no_init, test_get_frame_type_name)
{
	zassert_str_equal(get_frame_type_name(HTTP2_DATA_FRAME), "DATA",
			  "Unexpected frame type");
	zassert_str_equal(get_frame_type_name(HTTP2_HEADERS_FRAME),
			  "HEADERS", "Unexpected frame type");
	zassert_str_equal(get_frame_type_name(HTTP2_PRIORITY_FRAME),
			  "PRIORITY", "Unexpected frame type");
	zassert_str_equal(get_frame_type_name(HTTP2_RST_STREAM_FRAME),
			  "RST_STREAM", "Unexpected frame type");
	zassert_str_equal(get_frame_type_name(HTTP2_SETTINGS_FRAME),
			  "SETTINGS", "Unexpected frame type");
	zassert_str_equal(get_frame_type_name(HTTP2_PUSH_PROMISE_FRAME),
			  "PUSH_PROMISE", "Unexpected frame type");
	zassert_str_equal(get_frame_type_name(HTTP2_PING_FRAME), "PING",
			  "Unexpected frame type");
	zassert_str_equal(get_frame_type_name(HTTP2_GOAWAY_FRAME),
			  "GOAWAY", "Unexpected frame type");
	zassert_str_equal(get_frame_type_name(HTTP2_WINDOW_UPDATE_FRAME),
			  "WINDOW_UPDATE", "Unexpected frame type");
	zassert_str_equal(get_frame_type_name(HTTP2_CONTINUATION_FRAME),
			  "CONTINUATION", "Unexpected frame type");
}

ZTEST(server_function_tests_no_init, test_parse_http_frames)
{
	static struct http_client_ctx ctx_client1;
	static struct http_client_ctx ctx_client2;
	struct http2_frame *frame;

	unsigned char buffer1[] = {
		0x00, 0x00, 0x0c, 0x04, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x64, 0x00,
		0x04, 0x00, 0x00, 0xff, 0xff, 0x00
	};
	unsigned char buffer2[] = {
		0x00, 0x00, 0x21, 0x01, 0x05, 0x00, 0x00, 0x00,
		0x01, 0x82, 0x84, 0x86, 0x41, 0x8a, 0x0b, 0xe2,
		0x5c, 0x0b, 0x89, 0x70, 0xdc, 0x78, 0x0f, 0x03,
		0x53, 0x03, 0x2a, 0x2f, 0x2a, 0x90, 0x7a, 0x8a,
		0xaa, 0x69, 0xd2, 0x9a, 0xc4, 0xc0, 0x57, 0x68,
		0x0b, 0x83
	};

	memcpy(ctx_client1.buffer, buffer1, sizeof(buffer1));
	memcpy(ctx_client2.buffer, buffer2, sizeof(buffer2));

	ctx_client1.cursor = ctx_client1.buffer;
	ctx_client1.data_len = ARRAY_SIZE(buffer1);

	ctx_client2.cursor = ctx_client2.buffer;
	ctx_client2.data_len = ARRAY_SIZE(buffer2);

	/* Test: Buffer with the first frame */
	int parser1 = parse_http_frame_header(&ctx_client1, ctx_client1.cursor,
					      ctx_client1.data_len);

	zassert_equal(parser1, 0, "Failed to parse the first frame");

	frame = &ctx_client1.current_frame;

	/* Validate frame details for the 1st frame */
	zassert_equal(frame->length, 0x0C, "Expected length for the 1st frame doesn't match");
	zassert_equal(frame->type, 0x04, "Expected type for the 1st frame doesn't match");
	zassert_equal(frame->flags, 0x00, "Expected flags for the 1st frame doesn't match");
	zassert_equal(frame->stream_identifier, 0x00,
		      "Expected stream_identifier for the 1st frame doesn't match");

	/* Test: Buffer with the second frame */
	int parser2 = parse_http_frame_header(&ctx_client2, ctx_client2.cursor,
					      ctx_client2.data_len);

	zassert_equal(parser2, 0, "Failed to parse the second frame");

	frame = &ctx_client2.current_frame;

	/* Validate frame details for the 2nd frame */
	zassert_equal(frame->length, 0x21, "Expected length for the 2nd frame doesn't match");
	zassert_equal(frame->type, 0x01, "Expected type for the 2nd frame doesn't match");
	zassert_equal(frame->flags, 0x05, "Expected flags for the 2nd frame doesn't match");
	zassert_equal(frame->stream_identifier, 0x01,
		      "Expected stream_identifier for the 2nd frame doesn't match");
}

static void http_server_tests_before(void *fixture)
{
	struct sockaddr_in sa;
	struct timeval optval = {
		.tv_sec = TIMEOUT_S,
		.tv_usec = 0,
	};
	int ret;

	ARG_UNUSED(fixture);

	ret = http_server_start();
	if (ret < 0) {
		printk("Failed to start the server\n");
		return;
	}

	ret = zsock_socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (ret < 0) {
		printk("Failed to create client socket (%d)\n", errno);
		return;
	}
	client_fd = ret;

	ret = zsock_setsockopt(client_fd, SOL_SOCKET, SO_RCVTIMEO, &optval,
			       sizeof(optval));
	if (ret < 0) {
		printk("Failed to set timeout (%d)\n", errno);
		return;
	}

	sa.sin_family = AF_INET;
	sa.sin_port = htons(SERVER_PORT);

	ret = zsock_inet_pton(AF_INET, SERVER_IPV4_ADDR, &sa.sin_addr.s_addr);
	if (ret != 1) {
		printk("inet_pton() failed to convert %s\n", SERVER_IPV4_ADDR);
		return;
	}

	ret = zsock_connect(client_fd, (struct sockaddr *)&sa, sizeof(sa));
	if (ret < 0) {
		printk("Failed to connect (%d)\n", errno);
	}
}

static void http_server_tests_after(void *fixture)
{
	ARG_UNUSED(fixture);

	if (client_fd >= 0) {
		(void)zsock_close(client_fd);
		client_fd = -1;
	}

	(void)http_server_stop();
}

ZTEST_SUITE(server_function_tests, NULL, NULL, http_server_tests_before,
	    http_server_tests_after, NULL);
ZTEST_SUITE(server_function_tests_no_init, NULL, NULL, NULL,
	    http_server_tests_after, NULL);
