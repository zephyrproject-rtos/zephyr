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

#define BUFFER_SIZE                    512
#define SERVER_IPV4_ADDR               "127.0.0.1"
#define SERVER_PORT                    8080
#define TIMEOUT_S                      1

#define UPGRADE_STREAM_ID              1
#define TEST_STREAM_ID_1               3
#define TEST_STREAM_ID_2               5

#define TEST_DYNAMIC_POST_PAYLOAD "Test dynamic POST"
#define TEST_DYNAMIC_GET_PAYLOAD "Test dynamic GET"
#define TEST_STATIC_PAYLOAD "Hello, World!"

/* Individual HTTP2 frames, used to compose requests. */
#define TEST_HTTP2_MAGIC \
	0x50, 0x52, 0x49, 0x20, 0x2a, 0x20, 0x48, 0x54, 0x54, 0x50, 0x2f, 0x32, \
	0x2e, 0x30, 0x0d, 0x0a, 0x0d, 0x0a, 0x53, 0x4d, 0x0d, 0x0a, 0x0d, 0x0a
#define TEST_HTTP2_SETTINGS \
	0x00, 0x00, 0x0c, 0x04, 0x00, 0x00, 0x00, 0x00,	0x00, \
	0x00, 0x03, 0x00, 0x00, 0x00, 0x64, 0x00, 0x04, 0x00, 0x00, 0xff, 0xff
#define TEST_HTTP2_SETTINGS_ACK \
	0x00, 0x00, 0x00, 0x04, 0x01, 0x00, 0x00, 0x00, 0x00
#define TEST_HTTP2_GOAWAY \
	0x00, 0x00, 0x08, 0x07, 0x00, 0x00, 0x00, 0x00, \
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
#define TEST_HTTP2_HEADERS_GET_ROOT_STREAM_1 \
	0x00, 0x00, 0x21, 0x01, 0x05, 0x00, 0x00, 0x00, TEST_STREAM_ID_1, \
	0x82, 0x84, 0x86, 0x41, 0x8a, 0x0b, 0xe2, 0x5c, 0x0b, 0x89, 0x70, 0xdc, \
	0x78, 0x0f, 0x03, 0x53, 0x03, 0x2a, 0x2f, 0x2a, 0x90, 0x7a, 0x8a, 0xaa, \
	0x69, 0xd2, 0x9a, 0xc4, 0xc0, 0x57, 0x68, 0x0b, 0x83
#define TEST_HTTP2_HEADERS_GET_INDEX_STREAM_2 \
	0x00, 0x00, 0x21, 0x01, 0x05, 0x00, 0x00, 0x00, TEST_STREAM_ID_2, \
	0x82, 0x85, 0x86, 0x41, 0x8a, 0x0b, 0xe2, 0x5c, 0x0b, 0x89, 0x70, 0xdc, \
	0x78, 0x0f, 0x03, 0x53, 0x03, 0x2a, 0x2f, 0x2a, 0x90, 0x7a, 0x8a, 0xaa, \
	0x69, 0xd2, 0x9a, 0xc4, 0xc0, 0x57, 0x68, 0x0b, 0x83
#define TEST_HTTP2_HEADERS_GET_DYNAMIC_STREAM_1 \
	0x00, 0x00, 0x2b, 0x01, 0x05, 0x00, 0x00, 0x00, TEST_STREAM_ID_1, \
	0x82, 0x86, 0x41, 0x87, 0x0b, 0xe2, 0x5c, 0x0b, 0x89, 0x70, 0xff, 0x04, \
	0x86, 0x62, 0x4f, 0x55, 0x0e, 0x93, 0x13, 0x7a, 0x88, 0x25, 0xb6, 0x50, \
	0xc3, 0xcb, 0xbc, 0xb8, 0x3f, 0x53, 0x03, 0x2a, 0x2f, 0x2a, 0x5f, 0x87, \
	0x49, 0x7c, 0xa5, 0x8a, 0xe8, 0x19, 0xaa
#define TEST_HTTP2_HEADERS_GET_DYNAMIC_STREAM_1_PADDED \
	0x00, 0x00, 0x3d, 0x01, 0x0d, 0x00, 0x00, 0x00, TEST_STREAM_ID_1, 0x11,\
	0x82, 0x86, 0x41, 0x87, 0x0b, 0xe2, 0x5c, 0x0b, 0x89, 0x70, 0xff, 0x04, \
	0x86, 0x62, 0x4f, 0x55, 0x0e, 0x93, 0x13, 0x7a, 0x88, 0x25, 0xb6, 0x50, \
	0xc3, 0xcb, 0xbc, 0xb8, 0x3f, 0x53, 0x03, 0x2a, 0x2f, 0x2a, 0x5f, 0x87, \
	0x49, 0x7c, 0xa5, 0x8a, 0xe8, 0x19, 0xaa, \
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, \
	0x00, 0x00, 0x00, 0x00, 0x00
#define TEST_HTTP2_HEADERS_POST_DYNAMIC_STREAM_1 \
	0x00, 0x00, 0x30, 0x01, 0x04, 0x00, 0x00, 0x00, TEST_STREAM_ID_1, \
	0x83, 0x86, 0x41, 0x87, 0x0b, 0xe2, 0x5c, 0x0b, 0x89, 0x70, 0xff, 0x04, \
	0x86, 0x62, 0x4f, 0x55, 0x0e, 0x93, 0x13, 0x7a, 0x88, 0x25, 0xb6, 0x50, \
	0xc3, 0xcb, 0xbc, 0xb8, 0x3f, 0x53, 0x03, 0x2a, 0x2f, 0x2a, 0x5f, 0x87, \
	0x49, 0x7c, 0xa5, 0x8a, 0xe8, 0x19, 0xaa, 0x0f, 0x0d, 0x02, 0x31, 0x37
#define TEST_HTTP2_HEADERS_POST_DYNAMIC_STREAM_1_PRIORITY \
	0x00, 0x00, 0x35, 0x01, 0x24, 0x00, 0x00, 0x00, TEST_STREAM_ID_1, \
	0x00, 0x00, 0x00, 0x00, 0x64, \
	0x83, 0x86, 0x41, 0x87, 0x0b, 0xe2, 0x5c, 0x0b, 0x89, 0x70, 0xff, 0x04, \
	0x86, 0x62, 0x4f, 0x55, 0x0e, 0x93, 0x13, 0x7a, 0x88, 0x25, 0xb6, 0x50, \
	0xc3, 0xcb, 0xbc, 0xb8, 0x3f, 0x53, 0x03, 0x2a, 0x2f, 0x2a, 0x5f, 0x87, \
	0x49, 0x7c, 0xa5, 0x8a, 0xe8, 0x19, 0xaa, 0x0f, 0x0d, 0x02, 0x31, 0x37
#define TEST_HTTP2_HEADERS_POST_DYNAMIC_STREAM_1_PRIORITY_PADDED \
	0x00, 0x00, 0x40, 0x01, 0x2c, 0x00, 0x00, 0x00, TEST_STREAM_ID_1, \
	0x0a, 0x00, 0x00, 0x00, 0x00, 0xc8,\
	0x83, 0x86, 0x41, 0x87, 0x0b, 0xe2, 0x5c, 0x0b, 0x89, 0x70, 0xff, 0x04, \
	0x86, 0x62, 0x4f, 0x55, 0x0e, 0x93, 0x13, 0x7a, 0x88, 0x25, 0xb6, 0x50, \
	0xc3, 0xcb, 0xbc, 0xb8, 0x3f, 0x53, 0x03, 0x2a, 0x2f, 0x2a, 0x5f, 0x87, \
	0x49, 0x7c, 0xa5, 0x8a, 0xe8, 0x19, 0xaa, 0x0f, 0x0d, 0x02, 0x31, 0x37, \
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
#define TEST_HTTP2_PARTIAL_HEADERS_POST_DYNAMIC_STREAM_1 \
	0x00, 0x00, 0x20, 0x01, 0x00, 0x00, 0x00, 0x00, TEST_STREAM_ID_1, \
	0x83, 0x86, 0x41, 0x87, 0x0b, 0xe2, 0x5c, 0x0b, 0x89, 0x70, 0xff, 0x04, \
	0x86, 0x62, 0x4f, 0x55, 0x0e, 0x93, 0x13, 0x7a, 0x88, 0x25, 0xb6, 0x50, \
	0xc3, 0xcb, 0xbc, 0xb8, 0x3f, 0x53, 0x03, 0x2a
#define TEST_HTTP2_CONTINUATION_POST_DYNAMIC_STREAM_1 \
	0x00, 0x00, 0x10, 0x09, 0x04, 0x00, 0x00, 0x00, TEST_STREAM_ID_1, \
	0x2f, 0x2a, 0x5f, 0x87, 0x49, 0x7c, 0xa5, 0x8a, 0xe8, 0x19, 0xaa, 0x0f, \
	0x0d, 0x02, 0x31, 0x37
#define TEST_HTTP2_DATA_POST_DYNAMIC_STREAM_1 \
	0x00, 0x00, 0x11, 0x00, 0x01, 0x00, 0x00, 0x00, TEST_STREAM_ID_1, \
	0x54, 0x65, 0x73, 0x74, 0x20, 0x64, 0x79, 0x6e, 0x61, 0x6d, 0x69, 0x63, \
	0x20, 0x50, 0x4f, 0x53, 0x54
#define TEST_HTTP2_DATA_POST_DYNAMIC_STREAM_1_PADDED \
	0x00, 0x00, 0x34, 0x00, 0x09, 0x00, 0x00, 0x00, TEST_STREAM_ID_1, 0x22, \
	0x54, 0x65, 0x73, 0x74, 0x20, 0x64, 0x79, 0x6e, 0x61, 0x6d, 0x69, 0x63, \
	0x20, 0x50, 0x4f, 0x53, 0x54, \
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, \
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, \
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
#define TEST_HTTP2_DATA_POST_DYNAMIC_STREAM_1_NO_END_STREAM \
	0x00, 0x00, 0x11, 0x00, 0x00, 0x00, 0x00, 0x00, TEST_STREAM_ID_1, \
	0x54, 0x65, 0x73, 0x74, 0x20, 0x64, 0x79, 0x6e, 0x61, 0x6d, 0x69, 0x63, \
	0x20, 0x50, 0x4f, 0x53, 0x54
#define TEST_HTTP2_TRAILING_HEADER_STREAM_1 \
	0x00, 0x00, 0x0c, 0x01, 0x05, 0x00, 0x00, 0x00, TEST_STREAM_ID_1, \
	0x40, 0x84, 0x92, 0xda, 0x69, 0xf5, 0x85, 0x9c, 0xa3, 0x90, 0xb6, 0x7f

static uint16_t test_http_service_port = SERVER_PORT;
HTTP_SERVICE_DEFINE(test_http_service, SERVER_IPV4_ADDR,
		    &test_http_service_port, 1, 10, NULL);

static const char static_resource_payload[] = TEST_STATIC_PAYLOAD;
struct http_resource_detail_static static_resource_detail = {
	.common = {
			.type = HTTP_RESOURCE_TYPE_STATIC,
			.bitmask_of_supported_http_methods = BIT(HTTP_GET),
		},
	.static_data = static_resource_payload,
	.static_data_len = sizeof(static_resource_payload) - 1,
};

HTTP_RESOURCE_DEFINE(static_resource, test_http_service, "/",
		     &static_resource_detail);

static uint8_t dynamic_payload[32];
static size_t dynamic_payload_len = sizeof(dynamic_payload);
static uint8_t dynamic_buffer[32];

static int dynamic_cb(struct http_client_ctx *client, enum http_data_status status,
		      uint8_t *buffer, size_t len, void *user_data)
{
	static size_t offset;
	size_t copy_len;
	int ret = 0;

	if (status == HTTP_SERVER_DATA_ABORTED) {
		offset = 0;
		return 0;
	}

	switch (client->method) {
	case HTTP_GET:
		copy_len = MIN(sizeof(dynamic_buffer),
			       dynamic_payload_len - offset);

		ret = copy_len;

		if (copy_len > 0) {
			memcpy(buffer, dynamic_payload + offset, copy_len);
			offset += copy_len;
		} else {
			/* All resource returned, reset progress. */
			offset = 0;
		}

		break;
	case HTTP_POST:
		if (len + offset > sizeof(dynamic_payload)) {
			return -ENOMEM;
		}

		if (len > 0) {
			memcpy(dynamic_payload + offset, buffer, len);
			offset += len;
		}

		if (status == HTTP_SERVER_DATA_FINAL) {
			/* All data received, reset progress. */
			dynamic_payload_len = offset;
			offset = 0;
		}

		break;
	default:
		return -ENOTSUP;
	}

	return ret;
}

struct http_resource_detail_dynamic dynamic_detail = {
	.common = {
		.type = HTTP_RESOURCE_TYPE_DYNAMIC,
		.bitmask_of_supported_http_methods =
			BIT(HTTP_GET) | BIT(HTTP_POST),
		.content_type = "text/plain",
	},
	.cb = dynamic_cb,
	.data_buffer = dynamic_buffer,
	.data_buffer_len = sizeof(dynamic_buffer),
	.user_data = NULL,
};

HTTP_RESOURCE_DEFINE(dynamic_resource, test_http_service, "/dynamic",
		     &dynamic_detail);

static int client_fd = -1;
static uint8_t buf[BUFFER_SIZE];

/* This function ensures that there's at least as much data as requested in
 * the buffer.
 */
static void test_read_data(size_t *offset, size_t need)
{
	int ret;

	while (*offset < need) {
		ret = zsock_recv(client_fd, buf + *offset, sizeof(buf) - *offset, 0);
		zassert_not_equal(ret, -1, "recv() failed (%d)", errno);
		*offset += ret;
		if (ret == 0) {
			break;
		}
	};

	zassert_true(*offset >= need, "Not all requested data received");
}

/* This function moves the remaining data in the buffer to the beginning. */
static void test_consume_data(size_t *offset, size_t consume)
{
	zassert_true(*offset >= consume, "Cannot consume more data than received");
	*offset -= consume;
	memmove(buf, buf + consume, *offset);
}

static void expect_http1_switching_protocols(size_t *offset)
{
	static const char switching_protocols[] =
		"HTTP/1.1 101 Switching Protocols\r\n"
		"Connection: Upgrade\r\n"
		"Upgrade: h2c\r\n"
		"\r\n";

	test_read_data(offset, sizeof(switching_protocols) - 1);
	zassert_mem_equal(buf, switching_protocols, sizeof(switching_protocols) - 1,
			  "Received data doesn't match expected response");
	test_consume_data(offset, sizeof(switching_protocols) - 1);
}

static void test_get_frame_header(size_t *offset, struct http2_frame *frame)
{
	test_read_data(offset, HTTP2_FRAME_HEADER_SIZE);

	frame->length = sys_get_be24(&buf[HTTP2_FRAME_LENGTH_OFFSET]);
	frame->type = buf[HTTP2_FRAME_TYPE_OFFSET];
	frame->flags = buf[HTTP2_FRAME_FLAGS_OFFSET];
	frame->stream_identifier = sys_get_be32(
				&buf[HTTP2_FRAME_STREAM_ID_OFFSET]);
	frame->stream_identifier &= HTTP2_FRAME_STREAM_ID_MASK;

	test_consume_data(offset, HTTP2_FRAME_HEADER_SIZE);
}

static void expect_http2_settings_frame(size_t *offset, bool ack)
{
	struct http2_frame frame;

	test_get_frame_header(offset, &frame);

	zassert_equal(frame.type, HTTP2_SETTINGS_FRAME, "Expected settings frame");
	zassert_equal(frame.stream_identifier, 0, "Settings frame stream ID must be 0");

	if (ack) {
		zassert_equal(frame.length, 0, "Invalid settings frame length");
		zassert_equal(frame.flags, HTTP2_FLAG_SETTINGS_ACK,
			      "Expected settings ACK flag");
	} else {
		zassert_equal(frame.length % sizeof(struct http2_settings_field), 0,
			      "Invalid settings frame length");
		zassert_equal(frame.flags, 0, "Expected no settings flags");

		/* Consume settings payload */
		test_read_data(offset, frame.length);
		test_consume_data(offset, frame.length);
	}
}

static void expect_http2_headers_frame(size_t *offset, int stream_id,
				       uint8_t flags)
{
	struct http2_frame frame;

	test_get_frame_header(offset, &frame);

	zassert_equal(frame.type, HTTP2_HEADERS_FRAME, "Expected headers frame");
	zassert_equal(frame.stream_identifier, stream_id,
		      "Invalid headers frame stream ID");
	zassert_equal(frame.flags, flags, "Unexpected flags received");

	/* Consume headers payload */
	test_read_data(offset, frame.length);
	test_consume_data(offset, frame.length);
}

static void expect_http2_data_frame(size_t *offset, int stream_id,
				    const uint8_t *payload, size_t payload_len,
				    uint8_t flags)
{
	struct http2_frame frame;

	test_get_frame_header(offset, &frame);

	zassert_equal(frame.type, HTTP2_DATA_FRAME, "Expected data frame");
	zassert_equal(frame.stream_identifier, stream_id,
		      "Invalid data frame stream ID");
	zassert_equal(frame.flags, flags, "Unexpected flags received");
	zassert_equal(frame.length, payload_len, "Unexpected data frame length");

	/* Verify data payload */
	test_read_data(offset, frame.length);
	zassert_mem_equal(buf, payload, payload_len, "Unexpected data payload");
	test_consume_data(offset, frame.length);
}

static void expect_http2_window_update_frame(size_t *offset, int stream_id)
{
	struct http2_frame frame;

	test_get_frame_header(offset, &frame);

	zassert_equal(frame.type, HTTP2_WINDOW_UPDATE_FRAME,
		      "Expected window update frame");
	zassert_equal(frame.stream_identifier, stream_id,
		      "Invalid window update frame stream ID");
	zassert_equal(frame.flags, 0, "Unexpected flags received");
	zassert_equal(frame.length, sizeof(uint32_t),
		      "Unexpected window update frame length");


	/* Consume window update payload */
	test_read_data(offset, frame.length);
	test_consume_data(offset, frame.length);
}

ZTEST(server_function_tests, test_http2_get_concurrent_streams)
{
	static const uint8_t request_get_2_streams[] = {
		TEST_HTTP2_MAGIC,
		TEST_HTTP2_SETTINGS,
		TEST_HTTP2_SETTINGS_ACK,
		TEST_HTTP2_HEADERS_GET_ROOT_STREAM_1,
		TEST_HTTP2_HEADERS_GET_INDEX_STREAM_2,
		TEST_HTTP2_GOAWAY,
	};
	size_t offset = 0;
	int ret;

	ret = zsock_send(client_fd, request_get_2_streams,
			 sizeof(request_get_2_streams), 0);
	zassert_not_equal(ret, -1, "send() failed (%d)", errno);

	memset(buf, 0, sizeof(buf));

	/* Settings frame is expected twice (server settings + settings ACK) */
	expect_http2_settings_frame(&offset, false);
	expect_http2_settings_frame(&offset, true);
	expect_http2_headers_frame(&offset, TEST_STREAM_ID_1,
				   HTTP2_FLAG_END_HEADERS);
	expect_http2_data_frame(&offset, TEST_STREAM_ID_1, TEST_STATIC_PAYLOAD,
				strlen(TEST_STATIC_PAYLOAD),
				HTTP2_FLAG_END_STREAM);
	expect_http2_headers_frame(&offset, TEST_STREAM_ID_2,
				   HTTP2_FLAG_END_HEADERS);
	expect_http2_data_frame(&offset, TEST_STREAM_ID_2, NULL, 0,
				HTTP2_FLAG_END_STREAM);
}

ZTEST(server_function_tests, test_http2_static_get)
{
	static const uint8_t request_get_static_simple[] = {
		TEST_HTTP2_MAGIC,
		TEST_HTTP2_SETTINGS,
		TEST_HTTP2_SETTINGS_ACK,
		TEST_HTTP2_HEADERS_GET_ROOT_STREAM_1,
		TEST_HTTP2_GOAWAY,
	};
	size_t offset = 0;
	int ret;

	ret = zsock_send(client_fd, request_get_static_simple,
			 sizeof(request_get_static_simple), 0);
	zassert_not_equal(ret, -1, "send() failed (%d)", errno);

	memset(buf, 0, sizeof(buf));

	expect_http2_settings_frame(&offset, false);
	expect_http2_settings_frame(&offset, true);
	expect_http2_headers_frame(&offset, TEST_STREAM_ID_1,
				   HTTP2_FLAG_END_HEADERS);
	expect_http2_data_frame(&offset, TEST_STREAM_ID_1, TEST_STATIC_PAYLOAD,
				strlen(TEST_STATIC_PAYLOAD),
				HTTP2_FLAG_END_STREAM);
}

ZTEST(server_function_tests, test_http1_static_upgrade_get)
{
	static const char http1_request[] =
		"GET / HTTP/1.1\r\n"
		"Host: 127.0.0.1:8080\r\n"
		"Accept: */*\r\n"
		"Accept-Encoding: deflate, gzip, br\r\n"
		"Connection: Upgrade, HTTP2-Settings\r\n"
		"Upgrade: h2c\r\n"
		"HTTP2-Settings: AAMAAABkAAQAoAAAAAIAAAAA\r\n"
		"\r\n";
	size_t offset = 0;
	int ret;

	ret = zsock_send(client_fd, http1_request, strlen(http1_request), 0);
	zassert_not_equal(ret, -1, "send() failed (%d)", errno);

	memset(buf, 0, sizeof(buf));

	/* Verify HTTP1 switching protocols response. */
	expect_http1_switching_protocols(&offset);

	/* Verify HTTP2 frames. */
	expect_http2_settings_frame(&offset, false);
	expect_http2_headers_frame(&offset, UPGRADE_STREAM_ID,
				   HTTP2_FLAG_END_HEADERS);
	expect_http2_data_frame(&offset, UPGRADE_STREAM_ID, TEST_STATIC_PAYLOAD,
				strlen(TEST_STATIC_PAYLOAD),
				HTTP2_FLAG_END_STREAM);
}

ZTEST(server_function_tests, test_http1_static_get)
{
	static const char http1_request[] =
		"GET / HTTP/1.1\r\n"
		"Host: 127.0.0.1:8080\r\n"
		"User-Agent: curl/7.68.0\r\n"
		"Accept: */*\r\n"
		"Accept-Encoding: deflate, gzip, br\r\n"
		"\r\n";
	static const char expected_response[] =
		"HTTP/1.1 200 OK\r\n"
		"Content-Type: text/html\r\n"
		"Content-Length: 13\r\n"
		"\r\n"
		TEST_STATIC_PAYLOAD;
	size_t offset = 0;
	int ret;

	ret = zsock_send(client_fd, http1_request, strlen(http1_request), 0);
	zassert_not_equal(ret, -1, "send() failed (%d)", errno);

	memset(buf, 0, sizeof(buf));

	test_read_data(&offset, sizeof(expected_response) - 1);
	zassert_mem_equal(buf, expected_response, sizeof(expected_response) - 1,
			  "Received data doesn't match expected response");
}

static void common_verify_http2_dynamic_post_request(const uint8_t *request,
						     size_t request_len)
{
	size_t offset = 0;
	int ret;

	ret = zsock_send(client_fd, request, request_len, 0);
	zassert_not_equal(ret, -1, "send() failed (%d)", errno);

	memset(buf, 0, sizeof(buf));

	expect_http2_settings_frame(&offset, false);
	expect_http2_settings_frame(&offset, true);
	expect_http2_headers_frame(&offset, TEST_STREAM_ID_1,
				   HTTP2_FLAG_END_HEADERS | HTTP2_FLAG_END_STREAM);

	zassert_equal(dynamic_payload_len, strlen(TEST_DYNAMIC_POST_PAYLOAD),
		      "Wrong dynamic resource length");
	zassert_mem_equal(dynamic_payload, TEST_DYNAMIC_POST_PAYLOAD,
			  dynamic_payload_len, "Wrong dynamic resource data");
}

ZTEST(server_function_tests, test_http2_dynamic_post)
{
	static const uint8_t request_post_dynamic[] = {
		TEST_HTTP2_MAGIC,
		TEST_HTTP2_SETTINGS,
		TEST_HTTP2_SETTINGS_ACK,
		TEST_HTTP2_HEADERS_POST_DYNAMIC_STREAM_1,
		TEST_HTTP2_DATA_POST_DYNAMIC_STREAM_1,
		TEST_HTTP2_GOAWAY,
	};

	common_verify_http2_dynamic_post_request(request_post_dynamic,
						 sizeof(request_post_dynamic));
}

ZTEST(server_function_tests, test_http1_dynamic_upgrade_post)
{
	static const char http1_request[] =
		"POST /dynamic HTTP/1.1\r\n"
		"Host: 127.0.0.1:8080\r\n"
		"User-Agent: curl/7.68.0\r\n"
		"Accept: */*\r\n"
		"Content-Length: 17\r\n"
		"Connection: Upgrade, HTTP2-Settings\r\n"
		"Upgrade: h2c\r\n"
		"HTTP2-Settings: AAMAAABkAAQAoAAAAAIAAAAA\r\n"
		"\r\n"
		TEST_DYNAMIC_POST_PAYLOAD;
	size_t offset = 0;
	int ret;

	ret = zsock_send(client_fd, http1_request, strlen(http1_request), 0);
	zassert_not_equal(ret, -1, "send() failed (%d)", errno);

	memset(buf, 0, sizeof(buf));

	/* Verify HTTP1 switching protocols response. */
	expect_http1_switching_protocols(&offset);

	/* Verify HTTP2 frames. */
	expect_http2_settings_frame(&offset, false);
	expect_http2_headers_frame(&offset, UPGRADE_STREAM_ID,
				   HTTP2_FLAG_END_HEADERS | HTTP2_FLAG_END_STREAM);

	zassert_equal(dynamic_payload_len, strlen(TEST_DYNAMIC_POST_PAYLOAD),
		      "Wrong dynamic resource length");
	zassert_mem_equal(dynamic_payload, TEST_DYNAMIC_POST_PAYLOAD,
			  dynamic_payload_len, "Wrong dynamic resource data");
}

ZTEST(server_function_tests, test_http1_dynamic_post)
{
	static const char http1_request[] =
		"POST /dynamic HTTP/1.1\r\n"
		"Host: 127.0.0.1:8080\r\n"
		"User-Agent: curl/7.68.0\r\n"
		"Accept: */*\r\n"
		"Content-Length: 17\r\n"
		"\r\n"
		TEST_DYNAMIC_POST_PAYLOAD;
	static const char expected_response[] =
		"HTTP/1.1 200 OK\r\n"
		"Content-Type: text/plain\r\n"
		"Transfer-Encoding: chunked\r\n"
		"\r\n"
		"0\r\n\r\n";
	size_t offset = 0;
	int ret;

	ret = zsock_send(client_fd, http1_request, strlen(http1_request), 0);
	zassert_not_equal(ret, -1, "send() failed (%d)", errno);

	memset(buf, 0, sizeof(buf));

	test_read_data(&offset, sizeof(expected_response) - 1);
	zassert_mem_equal(buf, expected_response, sizeof(expected_response) - 1,
			  "Received data doesn't match expected response");

	zassert_equal(dynamic_payload_len, strlen(TEST_DYNAMIC_POST_PAYLOAD),
		      "Wrong dynamic resource length");
	zassert_mem_equal(dynamic_payload, TEST_DYNAMIC_POST_PAYLOAD,
			  dynamic_payload_len, "Wrong dynamic resource data");
}

static void common_verify_http2_dynamic_get_request(const uint8_t *request,
						    size_t request_len)
{
	size_t offset = 0;
	int ret;

	dynamic_payload_len = strlen(TEST_DYNAMIC_GET_PAYLOAD);
	memcpy(dynamic_payload, TEST_DYNAMIC_GET_PAYLOAD, dynamic_payload_len);

	ret = zsock_send(client_fd, request, request_len, 0);
	zassert_not_equal(ret, -1, "send() failed (%d)", errno);

	memset(buf, 0, sizeof(buf));

	expect_http2_settings_frame(&offset, false);
	expect_http2_settings_frame(&offset, true);
	expect_http2_headers_frame(&offset, TEST_STREAM_ID_1,
				   HTTP2_FLAG_END_HEADERS);
	expect_http2_data_frame(&offset, TEST_STREAM_ID_1, TEST_DYNAMIC_GET_PAYLOAD,
				strlen(TEST_DYNAMIC_GET_PAYLOAD), 0);
	expect_http2_data_frame(&offset, TEST_STREAM_ID_1, NULL, 0,
				HTTP2_FLAG_END_STREAM);
}

ZTEST(server_function_tests, test_http2_dynamic_get)
{
	static const uint8_t request_get_dynamic[] = {
		TEST_HTTP2_MAGIC,
		TEST_HTTP2_SETTINGS,
		TEST_HTTP2_SETTINGS_ACK,
		TEST_HTTP2_HEADERS_GET_DYNAMIC_STREAM_1,
		TEST_HTTP2_GOAWAY,
	};

	common_verify_http2_dynamic_get_request(request_get_dynamic,
						sizeof(request_get_dynamic));
}

ZTEST(server_function_tests, test_http1_dynamic_upgrade_get)
{
	static const char http1_request[] =
		"GET /dynamic HTTP/1.1\r\n"
		"Host: 127.0.0.1:8080\r\n"
		"User-Agent: curl/7.68.0\r\n"
		"Accept: */*\r\n"
		"Accept-Encoding: deflate, gzip, br\r\n"
		"Connection: Upgrade, HTTP2-Settings\r\n"
		"Upgrade: h2c\r\n"
		"HTTP2-Settings: AAMAAABkAAQAoAAAAAIAAAAA\r\n"
		"\r\n";
	size_t offset = 0;
	int ret;

	dynamic_payload_len = strlen(TEST_DYNAMIC_GET_PAYLOAD);
	memcpy(dynamic_payload, TEST_DYNAMIC_GET_PAYLOAD, dynamic_payload_len);

	ret = zsock_send(client_fd, http1_request, strlen(http1_request), 0);
	zassert_not_equal(ret, -1, "send() failed (%d)", errno);

	memset(buf, 0, sizeof(buf));

	/* Verify HTTP1 switching protocols response. */
	expect_http1_switching_protocols(&offset);

	/* Verify HTTP2 frames. */
	expect_http2_settings_frame(&offset, false);
	expect_http2_headers_frame(&offset, UPGRADE_STREAM_ID,
				   HTTP2_FLAG_END_HEADERS);
	expect_http2_data_frame(&offset, UPGRADE_STREAM_ID, TEST_DYNAMIC_GET_PAYLOAD,
				strlen(TEST_DYNAMIC_GET_PAYLOAD), 0);
	expect_http2_data_frame(&offset, UPGRADE_STREAM_ID, NULL, 0,
				HTTP2_FLAG_END_STREAM);
}

ZTEST(server_function_tests, test_http1_dynamic_get)
{
	static const char http1_request[] =
		"GET /dynamic HTTP/1.1\r\n"
		"Host: 127.0.0.1:8080\r\n"
		"User-Agent: curl/7.68.0\r\n"
		"Accept: */*\r\n"
		"Accept-Encoding: deflate, gzip, br\r\n"
		"\r\n";
	static const char expected_response[] =
		"HTTP/1.1 200 OK\r\n"
		"Content-Type: text/plain\r\n"
		"Transfer-Encoding: chunked\r\n"
		"\r\n"
		"10\r\n" TEST_DYNAMIC_GET_PAYLOAD "\r\n"
		"0\r\n\r\n";
	size_t offset = 0;
	int ret;

	dynamic_payload_len = strlen(TEST_DYNAMIC_GET_PAYLOAD);
	memcpy(dynamic_payload, TEST_DYNAMIC_GET_PAYLOAD, dynamic_payload_len);

	ret = zsock_send(client_fd, http1_request, strlen(http1_request), 0);
	zassert_not_equal(ret, -1, "send() failed (%d)", errno);

	memset(buf, 0, sizeof(buf));

	test_read_data(&offset, sizeof(expected_response) - 1);
	zassert_mem_equal(buf, expected_response, sizeof(expected_response) - 1,
			  "Received data doesn't match expected response");
}

ZTEST(server_function_tests, test_http1_connection_close)
{
	static const char http1_request_1[] =
		"GET / HTTP/1.1\r\n"
		"Host: 127.0.0.1:8080\r\n"
		"User-Agent: curl/7.68.0\r\n"
		"Accept: */*\r\n"
		"Accept-Encoding: deflate, gzip, br\r\n"
		"\r\n";
	static const char http1_request_2[] =
		"GET / HTTP/1.1\r\n"
		"Host: 127.0.0.1:8080\r\n"
		"User-Agent: curl/7.68.0\r\n"
		"Accept: */*\r\n"
		"Accept-Encoding: deflate, gzip, br\r\n"
		"Connection: close\r\n"
		"\r\n";
	static const char expected_response[] =
		"HTTP/1.1 200 OK\r\n"
		"Content-Type: text/html\r\n"
		"Content-Length: 13\r\n"
		"\r\n"
		TEST_STATIC_PAYLOAD;
	size_t offset = 0;
	int ret;

	ret = zsock_send(client_fd, http1_request_1, strlen(http1_request_1), 0);
	zassert_not_equal(ret, -1, "send() failed (%d)", errno);

	memset(buf, 0, sizeof(buf));

	test_read_data(&offset, sizeof(expected_response) - 1);
	zassert_mem_equal(buf, expected_response, sizeof(expected_response) - 1,
			  "Received data doesn't match expected response");
	test_consume_data(&offset, sizeof(expected_response) - 1);

	/* With no connection: close, the server shall serve another request on
	 * the same connection.
	 */
	ret = zsock_send(client_fd, http1_request_2, strlen(http1_request_2), 0);
	zassert_not_equal(ret, -1, "send() failed (%d)", errno);

	memset(buf, 0, sizeof(buf));

	test_read_data(&offset, sizeof(expected_response) - 1);
	zassert_mem_equal(buf, expected_response, sizeof(expected_response) - 1,
			  "Received data doesn't match expected response");
	test_consume_data(&offset, sizeof(expected_response) - 1);

	/* Second request included connection: close, so we should expect the
	 * connection to be closed now.
	 */
	ret = zsock_recv(client_fd, buf, sizeof(buf), 0);
	zassert_equal(ret, 0, "Connection should've been closed");
}

ZTEST(server_function_tests, test_http2_post_data_with_padding)
{
	static const uint8_t request_post_dynamic[] = {
		TEST_HTTP2_MAGIC,
		TEST_HTTP2_SETTINGS,
		TEST_HTTP2_SETTINGS_ACK,
		TEST_HTTP2_HEADERS_POST_DYNAMIC_STREAM_1,
		TEST_HTTP2_DATA_POST_DYNAMIC_STREAM_1_PADDED,
		TEST_HTTP2_GOAWAY,
	};

	common_verify_http2_dynamic_post_request(request_post_dynamic,
						 sizeof(request_post_dynamic));
}

ZTEST(server_function_tests, test_http2_post_headers_with_priority)
{
	static const uint8_t request_post_dynamic[] = {
		TEST_HTTP2_MAGIC,
		TEST_HTTP2_SETTINGS,
		TEST_HTTP2_SETTINGS_ACK,
		TEST_HTTP2_HEADERS_POST_DYNAMIC_STREAM_1_PRIORITY,
		TEST_HTTP2_DATA_POST_DYNAMIC_STREAM_1_PADDED,
		TEST_HTTP2_GOAWAY,
	};

	common_verify_http2_dynamic_post_request(request_post_dynamic,
						 sizeof(request_post_dynamic));
}

ZTEST(server_function_tests, test_http2_post_headers_with_priority_and_padding)
{
	static const uint8_t request_post_dynamic[] = {
		TEST_HTTP2_MAGIC,
		TEST_HTTP2_SETTINGS,
		TEST_HTTP2_SETTINGS_ACK,
		TEST_HTTP2_HEADERS_POST_DYNAMIC_STREAM_1_PRIORITY_PADDED,
		TEST_HTTP2_DATA_POST_DYNAMIC_STREAM_1_PADDED,
		TEST_HTTP2_GOAWAY,
	};

	common_verify_http2_dynamic_post_request(request_post_dynamic,
						 sizeof(request_post_dynamic));
}

ZTEST(server_function_tests, test_http2_post_headers_with_continuation)
{
	static const uint8_t request_post_dynamic[] = {
		TEST_HTTP2_MAGIC,
		TEST_HTTP2_SETTINGS,
		TEST_HTTP2_SETTINGS_ACK,
		TEST_HTTP2_PARTIAL_HEADERS_POST_DYNAMIC_STREAM_1,
		TEST_HTTP2_CONTINUATION_POST_DYNAMIC_STREAM_1,
		TEST_HTTP2_DATA_POST_DYNAMIC_STREAM_1,
		TEST_HTTP2_GOAWAY,
	};

	common_verify_http2_dynamic_post_request(request_post_dynamic,
						 sizeof(request_post_dynamic));
}

ZTEST(server_function_tests, test_http2_post_missing_continuation)
{
	static const uint8_t request_post_dynamic[] = {
		TEST_HTTP2_MAGIC,
		TEST_HTTP2_SETTINGS,
		TEST_HTTP2_SETTINGS_ACK,
		TEST_HTTP2_PARTIAL_HEADERS_POST_DYNAMIC_STREAM_1,
		TEST_HTTP2_DATA_POST_DYNAMIC_STREAM_1,
		TEST_HTTP2_GOAWAY,
	};
	size_t offset = 0;
	int ret;

	memset(buf, 0, sizeof(buf));

	ret = zsock_send(client_fd, request_post_dynamic,
			 sizeof(request_post_dynamic), 0);
	zassert_not_equal(ret, -1, "send() failed (%d)", errno);

	/* Expect settings, but processing headers (and lack of continuation
	 * frame) should break the stream, and trigger disconnect.
	 */
	expect_http2_settings_frame(&offset, false);
	expect_http2_settings_frame(&offset, true);

	ret = zsock_recv(client_fd, buf, sizeof(buf), 0);
	zassert_equal(ret, 0, "Connection should've been closed");
}

ZTEST(server_function_tests, test_http2_post_trailing_headers)
{
	static const uint8_t request_post_dynamic[] = {
		TEST_HTTP2_MAGIC,
		TEST_HTTP2_SETTINGS,
		TEST_HTTP2_SETTINGS_ACK,
		TEST_HTTP2_HEADERS_POST_DYNAMIC_STREAM_1,
		TEST_HTTP2_DATA_POST_DYNAMIC_STREAM_1_NO_END_STREAM,
		TEST_HTTP2_TRAILING_HEADER_STREAM_1,
		TEST_HTTP2_GOAWAY,
	};
	size_t offset = 0;
	int ret;

	ret = zsock_send(client_fd, request_post_dynamic,
			 sizeof(request_post_dynamic), 0);
	zassert_not_equal(ret, -1, "send() failed (%d)", errno);

	memset(buf, 0, sizeof(buf));

	expect_http2_settings_frame(&offset, false);
	expect_http2_settings_frame(&offset, true);
	/* In this case order is reversed, data frame had not END_STREAM flag.
	 * Because of this, reply will only be sent after processing the final
	 * trailing headers frame, but this will be preceded by window update
	 * after processing the data frame.
	 */
	expect_http2_window_update_frame(&offset, TEST_STREAM_ID_1);
	expect_http2_window_update_frame(&offset, 0);
	expect_http2_headers_frame(&offset, TEST_STREAM_ID_1,
				   HTTP2_FLAG_END_HEADERS | HTTP2_FLAG_END_STREAM);

	zassert_equal(dynamic_payload_len, strlen(TEST_DYNAMIC_POST_PAYLOAD),
		      "Wrong dynamic resource length");
	zassert_mem_equal(dynamic_payload, TEST_DYNAMIC_POST_PAYLOAD,
			  dynamic_payload_len, "Wrong dynamic resource data");
}

ZTEST(server_function_tests, test_http2_get_headers_with_padding)
{
	static const uint8_t request_get_dynamic[] = {
		TEST_HTTP2_MAGIC,
		TEST_HTTP2_SETTINGS,
		TEST_HTTP2_SETTINGS_ACK,
		TEST_HTTP2_HEADERS_GET_DYNAMIC_STREAM_1_PADDED,
		TEST_HTTP2_GOAWAY,
	};

	common_verify_http2_dynamic_get_request(request_get_dynamic,
						sizeof(request_get_dynamic));
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

	memset(dynamic_payload, 0, sizeof(dynamic_payload));
	dynamic_payload_len = 0;

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
