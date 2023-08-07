/*
 * Copyright (c) 2023, Emna Rekik
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "server_functions.h"

#include <string.h>

#include <zephyr/net/socket.h>
#include <zephyr/posix/sys/eventfd.h>
#include <zephyr/ztest.h>

#define SUPPORT_BACKWARD_COMPATIBILITY 1
#define SUPPORT_HTTP2_UPGRADE 2
#define BUFFER_SIZE 256
#define STACK_SIZE 8192
#define MY_IPV4_ADDR "127.0.0.1"
#define PORT 8000
#define TIMEOUT 1000

static struct k_sem server_sem;

static K_THREAD_STACK_DEFINE(server_stack, STACK_SIZE);

static struct k_thread server_thread;

static const unsigned char inc_file[] = {
#include "index.html.gz.inc"
};

static const unsigned char compressed_inc_file[] = {
	0x1f, 0x8b, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0xff, 0x35, 0x8e, 0xc1, 0x0a, 0xc2,
	0x30, 0x0c, 0x86, 0xef, 0x7d, 0x8a, 0xec, 0x05, 0x2c, 0xbb, 0x87, 0x5c, 0x54, 0xf0, 0xe0,
	0x50, 0x58, 0x41, 0x3c, 0x4e, 0x17, 0x69, 0x21, 0xa5, 0x65, 0x2d, 0x42, 0xdf, 0xde, 0xba,
	0x6e, 0x21, 0x10, 0xf8, 0xf9, 0xbe, 0x9f, 0x60, 0x77, 0xba, 0x1d, 0xcd, 0xf3, 0x7e, 0x06,
	0x9b, 0xbd, 0x90, 0xc2, 0xfd, 0xf0, 0x34, 0x93, 0x82, 0x3a, 0x98, 0x5d, 0x16, 0xa6, 0xa1,
	0xc0, 0xe8, 0x7c, 0x14, 0x86, 0x91, 0x97, 0x2f, 0x2f, 0xa8, 0x5b, 0xae, 0x50, 0x37, 0x16,
	0x5f, 0x61, 0x2e, 0x9b, 0x62, 0x7b, 0x7a, 0xb0, 0xbc, 0x83, 0x67, 0xc8, 0x01, 0x7c, 0x81,
	0xd4, 0xd4, 0xb4, 0xaa, 0x5d, 0x55, 0xfa, 0x8d, 0x8c, 0x64, 0xac, 0x4b, 0x50, 0x77, 0xda,
	0xa1, 0x8b, 0x19, 0xae, 0xf0, 0x71, 0xc2, 0x07, 0xd4, 0xf1, 0xdf, 0xdf, 0x8a, 0xab, 0xb4,
	0xbe, 0xf6, 0x03, 0xea, 0x2d, 0x11, 0x5c, 0xb2, 0x00,
};

/* Magic, SETTINGS[0], HEADERS[1]: GET /, HEADERS[3]: GET /index.html */
static const unsigned char Frame1[] = {
	0x50, 0x52, 0x49, 0x20, 0x2a, 0x20, 0x48, 0x54, 0x54, 0x50, 0x2f, 0x32, 0x2e, 0x30, 0x0d,
	0x0a, 0x0d, 0x0a, 0x53, 0x4d, 0x0d, 0x0a, 0x0d, 0x0a, 0x00, 0x00, 0x0c, 0x04, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x64, 0x00, 0x04, 0x00, 0x00, 0xff, 0xff,
	0x00, 0x00, 0x21, 0x01, 0x05, 0x00, 0x00, 0x00, 0x01, 0x82, 0x84, 0x86, 0x41, 0x8a, 0x0b,
	0xe2, 0x5c, 0x0b, 0x89, 0x70, 0xdc, 0x78, 0x0f, 0x03, 0x53, 0x03, 0x2a, 0x2f, 0x2a, 0x90,
	0x7a, 0x8a, 0xaa, 0x69, 0xd2, 0x9a, 0xc4, 0xc0, 0x57, 0x68, 0x0b, 0x83, 0x00, 0x00, 0x07,
	0x01, 0x05, 0x00, 0x00, 0x00, 0x03, 0x82, 0x85, 0x86, 0xc0, 0xbf, 0x90, 0xbe
};

/* SETTINGS[0] */
static const unsigned char Frame2[] = {0x00, 0x00, 0x00, 0x04, 0x01,
				       0x00, 0x00, 0x00, 0x00};

/* GOAWAY[0] */
static const unsigned char Frame3[] = {0x00, 0x00, 0x08, 0x07, 0x00, 0x00,
				       0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
				       0x00, 0x00, 0x00, 0x00, 0x00};

ZTEST_SUITE(server_function_tests, NULL, NULL, NULL, NULL, NULL);

static void server_thread_fn_streams(void *arg0, void *arg1, void *arg2)
{
	struct http2_server_ctx *ctx = (struct http2_server_ctx *)arg0;

	k_thread_name_set(k_current_get(), "server");

	k_sem_give(&server_sem);

	ctx->infinite = 0;
	for (int i = 0; i < 2; i++) {
		http2_server_start(ctx);
	}
}

static void test_streams(void)
{
	int r;
	int client_fd;
	int proto = IPPROTO_TCP;
	char *addrstrp;
	k_tid_t server_thread_id;
	struct sockaddr_in sa;
	char addrstr[64];

	k_sem_init(&server_sem, 0, 1);

	struct http2_server_config config;
	struct http2_server_ctx ctx;

	config.port = PORT;
	config.address_family = AF_INET;

	int server_fd = http2_server_init(&ctx, &config);

	zassert_true(server_fd >= 0, "Failed to create server socket");

	server_thread_id = k_thread_create(
	    &server_thread, server_stack, STACK_SIZE, server_thread_fn_streams,
	    &ctx, NULL, NULL, K_PRIO_PREEMPT(8), 0, K_NO_WAIT);

	r = k_sem_take(&server_sem, K_MSEC(TIMEOUT));
	zassert_equal(0, r, "failed to synchronize with server thread (%d)", r);

	k_thread_name_set(k_current_get(), "client");

	r = socket(AF_INET, SOCK_STREAM, proto);
	zassert_not_equal(r, -1, "failed to create client socket (%d)", errno);
	client_fd = r;

	sa.sin_family = AF_INET;
	sa.sin_port = htons(PORT);

	r = inet_pton(AF_INET, MY_IPV4_ADDR, &sa.sin_addr.s_addr);
	zassert_not_equal(-1, r, "inet_pton() failed (%d)", errno);
	zassert_not_equal(0, r, "%s is not a valid IPv4 address", MY_IPV4_ADDR);
	zassert_equal(1, r, "inet_pton() failed to convert %s", MY_IPV4_ADDR);

	memset(addrstr, '\0', sizeof(addrstr));
	addrstrp =
	    (char *)inet_ntop(AF_INET, &sa.sin_addr, addrstr, sizeof(addrstr));
	zassert_not_equal(addrstrp, NULL, "inet_ntop() failed (%d)", errno);

	r = connect(client_fd, (struct sockaddr *)&sa, sizeof(sa));
	zassert_not_equal(r, -1, "failed to connect (%d)", errno);

	r = send(client_fd, Frame1, sizeof(Frame1), 0);
	zassert_not_equal(r, -1, "send() failed (%d)", errno);

	memset(addrstr, 0, sizeof(addrstr));
	r = recv(client_fd, addrstr, sizeof(addrstr), 0);
	zassert_not_equal(r, -1, "recv() failed (%d)", errno);

	r = send(client_fd, Frame2, sizeof(Frame2), 0);
	zassert_not_equal(r, -1, "send() failed (%d)", errno);

	memset(addrstr, 0, sizeof(addrstr));
	r = recv(client_fd, addrstr, sizeof(addrstr), 0);
	zassert_not_equal(r, -1, "recv() failed (%d)", errno);

	unsigned int length;
	uint8_t type;
	uint32_t stream_id;

	length = (addrstr[0] << 16) | (addrstr[1] << 8) | addrstr[2];
	length += 9;
	type = addrstr[3];
	stream_id = (addrstr[5] << 24) | (addrstr[6] << 16) |
		    (addrstr[7] << 8) | addrstr[8];
	stream_id &= 0x7fffffff;

	zassert_true((type == 0x4 && stream_id == 0),
		     "Expected a SETTINGS frame with stream ID 0");

	r -= length;
	memmove(addrstr, addrstr + length, r);

	length = (addrstr[0] << 16) | (addrstr[1] << 8) | addrstr[2];
	length += 9;
	type = addrstr[3];
	stream_id = (addrstr[5] << 24) | (addrstr[6] << 16) |
		    (addrstr[7] << 8) | addrstr[8];
	stream_id &= 0x7fffffff;

	zassert_true((type == 0x1 && stream_id == 1),
		     "Expected a HEADERS frame with stream ID 1");

	r -= length;
	memmove(addrstr, addrstr + length, r);

	length = (addrstr[0] << 16) | (addrstr[1] << 8) | addrstr[2];
	length += 9;
	type = addrstr[3];
	stream_id = (addrstr[5] << 24) | (addrstr[6] << 16) |
		    (addrstr[7] << 8) | addrstr[8];
	stream_id &= 0x7fffffff;

	zassert_true((type == 0x1 && stream_id == 3),
		     "Expected a HEADERS frame with stream ID 3");

	r = send(client_fd, Frame3, sizeof(Frame3), 0);
	zassert_not_equal(r, -1, "send() failed (%d)", errno);

	memset(addrstr, 0, sizeof(addrstr));
	r = recv(client_fd, addrstr, sizeof(addrstr), 0);
	zassert_not_equal(r, -1, "recv() failed (%d)", errno);

	r = close(client_fd);
	zassert_not_equal(-1, r, "close() failed on the client fd (%d)", errno);

	r = close(server_fd);
	zassert_not_equal(-1, r, "close() failed on the server fd (%d)", errno);

	r = k_thread_join(&server_thread, K_FOREVER);
	zassert_equal(0, r, "k_thread_join() failed (%d)", r);
}

ZTEST(server_function_tests, test_http2_concurrent_streams) { test_streams(); }

static void server_thread_fn(void *arg0, void *arg1, void *arg2)
{
	const int server_fd = POINTER_TO_INT(arg0);
	const int test_support = POINTER_TO_INT(arg1);

	int r;
	int client_fd;
	socklen_t addrlen;
	char addrstr[BUFFER_SIZE];
	struct sockaddr_in sa;
	int support = 0;
	const char *preface = "PRI * HTTP/2.0\r\n\r\nSM\r\n\r\n";

	k_thread_name_set(k_current_get(), "server");

	memset(&sa, 0, sizeof(sa));
	addrlen = sizeof(sa);

	k_sem_give(&server_sem);

	client_fd = accept_new_client(server_fd);

	if (test_support == SUPPORT_BACKWARD_COMPATIBILITY) {

		memset(addrstr, '\0', sizeof(addrstr));
		r = recv(client_fd, addrstr, sizeof(addrstr), 0);
		zassert_not_equal(r, -1, "recv() failed (%d)", errno);

		if (strncmp(addrstr, preface, strlen(preface)) != 0)
			support = SUPPORT_BACKWARD_COMPATIBILITY;

		zassert_equal(support, SUPPORT_BACKWARD_COMPATIBILITY,
			      "Server does not supprt backward compatibility");
	} else if (test_support == SUPPORT_HTTP2_UPGRADE) {

		memset(addrstr, '\0', sizeof(addrstr));
		r = recv(client_fd, addrstr, sizeof(addrstr), 0);
		zassert_not_equal(r, -1, "recv() failed (%d)", errno);

		if (strncmp(addrstr, preface, strlen(preface)) == 0)
			support = SUPPORT_HTTP2_UPGRADE;

		zassert_equal(support, SUPPORT_HTTP2_UPGRADE,
			      "Server does not supprt HTTP1 to HTTP2 upgrade");
	}
	r = close(client_fd);
	zassert_not_equal(r, -1, "close() failed on the server fd (%d)", errno);
}

static void test_common(int test_support)
{
	int r;
	int client_fd;
	int proto = IPPROTO_TCP;
	char *addrstrp;
	k_tid_t server_thread_id;
	struct sockaddr_in sa;
	char addrstr[INET_ADDRSTRLEN];

	k_sem_init(&server_sem, 0, 1);

	struct http2_server_config config;
	struct http2_server_ctx ctx;

	config.port = PORT;
	config.address_family = AF_INET;

	int server_fd = http2_server_init(&ctx, &config);

	zassert_true(server_fd >= 0, "Failed to create server socket");

	server_thread_id = k_thread_create(
	    &server_thread, server_stack, STACK_SIZE, server_thread_fn,
	    INT_TO_POINTER(server_fd), INT_TO_POINTER(test_support), NULL,
	    K_PRIO_PREEMPT(8), 0, K_NO_WAIT);

	r = k_sem_take(&server_sem, K_MSEC(TIMEOUT));
	zassert_equal(0, r, "failed to synchronize with server thread (%d)", r);

	k_thread_name_set(k_current_get(), "client");

	r = socket(AF_INET, SOCK_STREAM, proto);
	zassert_not_equal(r, -1, "failed to create client socket (%d)", errno);
	client_fd = r;

	sa.sin_family = AF_INET;
	sa.sin_port = htons(PORT);

	r = inet_pton(AF_INET, MY_IPV4_ADDR, &sa.sin_addr.s_addr);
	zassert_not_equal(-1, r, "inet_pton() failed (%d)", errno);
	zassert_not_equal(0, r, "%s is not a valid IPv4 address", MY_IPV4_ADDR);
	zassert_equal(1, r, "inet_pton() failed to convert %s", MY_IPV4_ADDR);

	memset(addrstr, '\0', sizeof(addrstr));
	addrstrp =
	    (char *)inet_ntop(AF_INET, &sa.sin_addr, addrstr, sizeof(addrstr));
	zassert_not_equal(addrstrp, NULL, "inet_ntop() failed (%d)", errno);

	r = connect(client_fd, (struct sockaddr *)&sa, sizeof(sa));
	zassert_not_equal(r, -1, "failed to connect (%d)", errno);

	if (test_support == SUPPORT_BACKWARD_COMPATIBILITY) {

		char *http1_request = "GET / HTTP/1.1\r\n\r\n";

		r = send(client_fd, http1_request, strlen(http1_request), 0);
		zassert_not_equal(r, -1, "send() failed (%d)", errno);
	} else if (test_support == SUPPORT_HTTP2_UPGRADE) {

		char *http2_upgrade_request =
		    "PRI * HTTP/2.0\r\n\r\nSM\r\n\r\n";

		r = send(client_fd, http2_upgrade_request,
			 strlen(http2_upgrade_request), 0);
		zassert_not_equal(r, -1, "send() failed (%d)", errno);
	}

	memset(addrstr, 0, sizeof(addrstr));
	r = recv(client_fd, addrstr, sizeof(addrstr), 0);
	zassert_not_equal(r, -1, "recv() failed (%d)", errno);

	r = close(client_fd);
	zassert_not_equal(-1, r, "close() failed on the client fd (%d)", errno);

	r = close(server_fd);
	zassert_not_equal(-1, r, "close() failed on the server fd (%d)", errno);

	r = k_thread_join(&server_thread, K_FOREVER);
	zassert_equal(0, r, "k_thread_join() failed (%d)", r);
}

ZTEST(server_function_tests, test_http2_upgrade)
{
	test_common(SUPPORT_HTTP2_UPGRADE);
}

ZTEST(server_function_tests, test_backward_compatibility)
{
	test_common(SUPPORT_BACKWARD_COMPATIBILITY);
}

ZTEST(server_function_tests, test_gen_gz_inc_file)
{
	const char *data;
	size_t len;

	data = inc_file;
	len = sizeof(inc_file);
	len = len - 2;

	zassert_equal(len, sizeof(compressed_inc_file),
		      "Invalid compressed file size");

	int result = memcmp(data, compressed_inc_file, len);

	zassert_equal(
	    result, 0,
	    "inc_file and compressed_inc_file contents are not identical");
}

ZTEST(server_function_tests, test_http2_support_ipv6)
{
	struct http2_server_config config;
	struct http2_server_ctx ctx;

	config.port = 8080;
	config.address_family = AF_INET6;

	int server_fd = http2_server_init(&ctx, &config);

	/* Check that the function returned a valid file descriptor */
	zassert_true(server_fd >= 0,
		     "Failed to initialize HTTP/2 server with IPv6");

	close(server_fd);
}

ZTEST(server_function_tests, test_http2_support_ipv4)
{
	struct http2_server_config config;
	struct http2_server_ctx ctx;

	config.port = 8080;
	config.address_family = AF_INET;

	int server_fd = http2_server_init(&ctx, &config);

	/* Check that the function returned a valid file descriptor */
	zassert_true(server_fd >= 0,
		     "Failed to initialize HTTP/2 server with IPv4");

	close(server_fd);
}

int http2_server_stop(struct http2_server_ctx *ctx)
{
	eventfd_write(ctx->event_fd, 1);

	return 0;
}

static void server_thread_stp_fn(void *arg0, void *arg1, void *arg2)
{
	struct http2_server_ctx *ctx = (struct http2_server_ctx *)arg0;

	k_sleep(K_SECONDS(2));

	ctx->infinite = 0;
	int program_status = http2_server_start(ctx);

	zassert_equal(program_status, 0,
		      "The server didn't shut down successfully");
}

ZTEST(server_function_tests, test_http2_server_stop)
{
	struct http2_server_config config;
	struct http2_server_ctx ctx;

	config.port = PORT;
	config.address_family = AF_INET;

	int server_fd = http2_server_init(&ctx, &config);

	zassert_true(server_fd >= 0, "Failed to create server socket");

	k_thread_create(&server_thread, server_stack, STACK_SIZE,
			server_thread_stp_fn, &ctx, NULL, NULL,
			K_PRIO_PREEMPT(8), 0, K_NO_WAIT);

	k_sleep(K_MSEC(100));

	http2_server_stop(&ctx);

	k_thread_join(&server_thread, K_FOREVER);

	close(server_fd);
}

ZTEST(server_function_tests, test_http2_server_init)
{
	struct http2_server_config config;
	struct http2_server_ctx ctx;

	config.port = 8080;
	config.address_family = AF_INET;

	int server_fd = http2_server_init(&ctx, &config);

	/* Check that the function returned a valid file descriptor */
	zassert_true(server_fd >= 0, "Failed to initiate server socket");

	close(server_fd);
}

ZTEST(server_function_tests, test_get_frame_type_name)
{
	zassert_equal(strcmp(get_frame_type_name(HTTP2_DATA_FRAME), "DATA"), 0,
		      "Unexpected frame type");
	zassert_equal(
	    strcmp(get_frame_type_name(HTTP2_HEADERS_FRAME), "HEADERS"), 0,
	    "Unexpected frame type");
	zassert_equal(
	    strcmp(get_frame_type_name(HTTP2_PRIORITY_FRAME), "PRIORITY"), 0,
	    "Unexpected frame type");
	zassert_equal(
	    strcmp(get_frame_type_name(HTTP2_RST_STREAM_FRAME), "RST_STREAM"),
	    0, "Unexpected frame type");
	zassert_equal(
	    strcmp(get_frame_type_name(HTTP2_SETTINGS_FRAME), "SETTINGS"), 0,
	    "Unexpected frame type");
	zassert_equal(strcmp(get_frame_type_name(HTTP2_PUSH_PROMISE_FRAME),
			     "PUSH_PROMISE"),
		      0, "Unexpected frame type");
	zassert_equal(strcmp(get_frame_type_name(HTTP2_PING_FRAME), "PING"), 0,
		      "Unexpected frame type");
	zassert_equal(strcmp(get_frame_type_name(HTTP2_GOAWAY_FRAME), "GOAWAY"),
		      0, "Unexpected frame type");
	zassert_equal(strcmp(get_frame_type_name(HTTP2_WINDOW_UPDATE_FRAME),
			     "WINDOW_UPDATE"),
		      0, "Unexpected frame type");
	zassert_equal(strcmp(get_frame_type_name(HTTP2_CONTINUATION_FRAME),
			     "CONTINUATION"),
		      0, "Unexpected frame type");
	zassert_equal(strcmp(get_frame_type_name(9999), "UNKNOWN"), 0,
		      "Unexpected frame type");
}

ZTEST(server_function_tests, test_parse_http2_frames)
{
	unsigned char buffer1[] = {
	    0x00, 0x00, 0x0c, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03,
	    0x00, 0x00, 0x00, 0x64, 0x00, 0x04, 0x00, 0x00, 0xff, 0xff, 0x00};
	unsigned char buffer2[] = {
	    0x00, 0x00, 0x21, 0x01, 0x05, 0x00, 0x00, 0x00, 0x01, 0x82, 0x84,
	    0x86, 0x41, 0x8a, 0x0b, 0xe2, 0x5c, 0x0b, 0x89, 0x70, 0xdc, 0x78,
	    0x0f, 0x03, 0x53, 0x03, 0x2a, 0x2f, 0x2a, 0x90, 0x7a, 0x8a, 0xaa,
	    0x69, 0xd2, 0x9a, 0xc4, 0xc0, 0x57, 0x68, 0x0b, 0x83};
	struct http2_frame frame;
	unsigned int buffer_len1 = ARRAY_SIZE(buffer1);
	unsigned int buffer_len2 = ARRAY_SIZE(buffer2);

	/* Test: Buffer with the first frame */
	int parser1 = parse_http2_frame(buffer1, buffer_len1, &frame);

	zassert_true(parser1, "Failed to parse the first frame");

	/* Validate frame details for the 1st frame */
	zassert_equal(frame.length, 0x0C,
		      "Expected length for the 1st frame doesn't match");
	zassert_equal(frame.type, 0x04,
		      "Expected type for the 1st frame doesn't match");
	zassert_equal(frame.flags, 0x00,
		      "Expected flags for the 1st frame doesn't match");
	zassert_equal(
	    frame.stream_identifier, 0x00,
	    "Expected stream_identifier for the 1st frame doesn't match");

	/* Test: Buffer with the second frame */
	int parser2 = parse_http2_frame(buffer2, buffer_len2, &frame);

	zassert_true(parser2, "Failed to parse the second frame");

	/* Validate frame details for the 2nd frame */
	zassert_equal(frame.length, 0x21,
		      "Expected length for the 2nd frame doesn't match");
	zassert_equal(frame.type, 0x01,
		      "Expected type for the 2nd frame doesn't match");
	zassert_equal(frame.flags, 0x05,
		      "Expected flags for the 2nd frame doesn't match");
	zassert_equal(
	    frame.stream_identifier, 0x01,
	    "Expected stream_identifier for the 2nd frame doesn't match");
}
