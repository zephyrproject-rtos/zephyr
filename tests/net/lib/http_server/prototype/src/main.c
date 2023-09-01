/*
 * Copyright (c) 2023, Emna Rekik
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "server_functions.h"

#include <string.h>

#include <zephyr/net/http/service.h>
#include <zephyr/net/socket.h>
#include <zephyr/posix/sys/eventfd.h>
#include <zephyr/ztest.h>

#define SUPPORT_BACKWARD_COMPATIBILITY 1
#define SUPPORT_HTTP_SERVER_UPGRADE    2
#define BUFFER_SIZE                    256
#define STACK_SIZE                     8192
#define MY_IPV4_ADDR                   "127.0.0.1"
#define SERVER_PORT                    8080
#define TIMEOUT                        1000

static struct k_sem server_sem;

static K_THREAD_STACK_DEFINE(server_stack, STACK_SIZE);

static struct k_thread server_thread;

/* Magic, SETTINGS[0], HEADERS[1]: GET /, HEADERS[3]: GET /index.html, SETTINGS[0], GOAWAY[0]*/
static const unsigned char Frame[] = {
	0x50, 0x52, 0x49, 0x20, 0x2a, 0x20, 0x48, 0x54, 0x54, 0x50, 0x2f, 0x32, 0x2e, 0x30, 0x0d,
	0x0a, 0x0d, 0x0a, 0x53, 0x4d, 0x0d, 0x0a, 0x0d, 0x0a, 0x00, 0x00, 0x0c, 0x04, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x64, 0x00, 0x04, 0x00, 0x00, 0xff, 0xff,
	0x00, 0x00, 0x21, 0x01, 0x05, 0x00, 0x00, 0x00, 0x01, 0x82, 0x84, 0x86, 0x41, 0x8a, 0x0b,
	0xe2, 0x5c, 0x0b, 0x89, 0x70, 0xdc, 0x78, 0x0f, 0x03, 0x53, 0x03, 0x2a, 0x2f, 0x2a, 0x90,
	0x7a, 0x8a, 0xaa, 0x69, 0xd2, 0x9a, 0xc4, 0xc0, 0x57, 0x68, 0x0b, 0x83, 0x00, 0x00, 0x07,
	0x01, 0x05, 0x00, 0x00, 0x00, 0x03, 0x82, 0x85, 0x86, 0xc0, 0xbf, 0x90, 0xbe, 0x00, 0x00,
	0x00, 0x04, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08, 0x07, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

static uint16_t test_http_service_port = htons(SERVER_PORT);
HTTP_SERVICE_DEFINE(test_http_service, CONFIG_NET_CONFIG_MY_IPV4_ADDR, &test_http_service_port, 1,
		    10, NULL);

static const char index_html_gz[] = "Hello, World!";
struct http_resource_detail_static index_html_gz_resource_detail = {
	.common = {
			.type = HTTP_RESOURCE_TYPE_STATIC,
			.bitmask_of_supported_http_methods = GET,
		},
	.static_data = index_html_gz,
	.static_data_len = sizeof(index_html_gz),
};

HTTP_RESOURCE_DEFINE(index_html_gz_resource, test_http_service, "/",
		     &index_html_gz_resource_detail);

static void server_thread_fn(void *arg0, void *arg1, void *arg2)
{
	struct http_server_ctx *ctx = (struct http_server_ctx *)arg0;

	k_thread_name_set(k_current_get(), "server");

	k_sem_give(&server_sem);

	http_server_start(ctx);
}

static void test_streams(void)
{
	int r;
	int client_fd;
	int proto = IPPROTO_TCP;
	char *addrstrp;
	k_tid_t server_thread_id;
	struct sockaddr_in sa;
	unsigned char addrstr[512];

	k_sem_init(&server_sem, 0, 1);

	struct http_server_ctx ctx;

	int server_fd = http_server_init(&ctx);

	zassert_true(server_fd >= 0, "Failed to create server socket");

	server_thread_id =
		k_thread_create(&server_thread, server_stack, STACK_SIZE, server_thread_fn, &ctx,
				NULL, NULL, K_PRIO_PREEMPT(8), 0, K_NO_WAIT);

	r = k_sem_take(&server_sem, K_MSEC(TIMEOUT));
	zassert_equal(0, r, "failed to synchronize with server thread (%d)", r);

	k_thread_name_set(k_current_get(), "client");

	r = socket(AF_INET, SOCK_STREAM, proto);
	zassert_not_equal(r, -1, "failed to create client socket (%d)", errno);
	client_fd = r;

	sa.sin_family = AF_INET;
	sa.sin_port = htons(SERVER_PORT);

	r = inet_pton(AF_INET, MY_IPV4_ADDR, &sa.sin_addr.s_addr);
	zassert_not_equal(-1, r, "inet_pton() failed (%d)", errno);
	zassert_not_equal(0, r, "%s is not a valid IPv4 address", MY_IPV4_ADDR);
	zassert_equal(1, r, "inet_pton() failed to convert %s", MY_IPV4_ADDR);

	memset(addrstr, '\0', sizeof(addrstr));
	addrstrp = (char *)inet_ntop(AF_INET, &sa.sin_addr, addrstr, sizeof(addrstr));
	zassert_not_equal(addrstrp, NULL, "inet_ntop() failed (%d)", errno);

	r = connect(client_fd, (struct sockaddr *)&sa, sizeof(sa));
	zassert_not_equal(r, -1, "failed to connect (%d)", errno);

	r = send(client_fd, Frame, sizeof(Frame), 0);
	zassert_not_equal(r, -1, "send() failed (%d)", errno);

	memset(addrstr, 0, sizeof(addrstr));
	r = recv(client_fd, addrstr, sizeof(addrstr), 0);
	zassert_not_equal(r, -1, "recv() failed (%d)", errno);

	memset(addrstr, 0, sizeof(addrstr));
	r = recv(client_fd, addrstr, sizeof(addrstr), 0);
	zassert_not_equal(r, -1, "recv() failed (%d)", errno);

	unsigned int length;
	uint8_t type;
	uint32_t stream_id;

	length = (addrstr[0] << 16) | (addrstr[1] << 8) | addrstr[2];
	length += 9;
	type = addrstr[3];
	stream_id = (addrstr[5] << 24) | (addrstr[6] << 16) | (addrstr[7] << 8) | addrstr[8];
	stream_id &= 0x7fffffff;

	zassert_true((type == 0x4 && stream_id == 0), "Expected a SETTINGS frame with stream ID 0");

	r -= length;
	memmove(addrstr, addrstr + length, r);

	length = (addrstr[0] << 16) | (addrstr[1] << 8) | addrstr[2];
	length += 9;
	type = addrstr[3];
	stream_id = (addrstr[5] << 24) | (addrstr[6] << 16) | (addrstr[7] << 8) | addrstr[8];
	stream_id &= 0x7fffffff;

	zassert_true((type == 0x1 && stream_id == 1), "Expected a HEADERS frame with stream ID 1");

	r -= length;
	memmove(addrstr, addrstr + length, r);

	length = (addrstr[0] << 16) | (addrstr[1] << 8) | addrstr[2];
	length += 9;
	type = addrstr[3];
	stream_id = (addrstr[5] << 24) | (addrstr[6] << 16) | (addrstr[7] << 8) | addrstr[8];
	stream_id &= 0x7fffffff;
	addrstr[9] = 0;

	zassert_true((type == 0x0 && stream_id == 1), "Expected a DATA frame with stream ID 1");

	r -= length;
	memmove(addrstr, addrstr + length, r);

	length = (addrstr[0] << 16) | (addrstr[1] << 8) | addrstr[2];
	length += 9;
	type = addrstr[3];
	stream_id = (addrstr[5] << 24) | (addrstr[6] << 16) | (addrstr[7] << 8) | addrstr[8];
	stream_id &= 0x7fffffff;

	zassert_true((type == 0x1 && stream_id == 3), "Expected a HEADERS frame with stream ID 3");

	r -= length;
	memmove(addrstr, addrstr + length, r);

	length = (addrstr[0] << 16) | (addrstr[1] << 8) | addrstr[2];
	length += 9;
	type = addrstr[3];
	stream_id = (addrstr[5] << 24) | (addrstr[6] << 16) | (addrstr[7] << 8) | addrstr[8];
	stream_id &= 0x7fffffff;

	zassert_true((type == 0x0 && stream_id == 3), "Expected a DATA frame with stream ID 3");

	r = close(client_fd);
	zassert_not_equal(-1, r, "close() failed on the client fd (%d)", errno);

	http_server_stop(&ctx);

	r = k_thread_join(&server_thread, K_FOREVER);
	zassert_equal(0, r, "k_thread_join() failed (%d)", r);

	r = close(server_fd);
	zassert_not_equal(-1, r, "close() failed on the server fd (%d)", errno);
}

ZTEST(server_function_tests, test_http_concurrent_streams)
{
	test_streams();
}

static void test_common(int test_support)
{
	int r;
	int client_fd;
	int proto = IPPROTO_TCP;
	char *addrstrp;
	k_tid_t server_thread_id;
	struct sockaddr_in sa;
	unsigned char addrstr[512];

	k_sem_init(&server_sem, 0, 1);

	struct http_server_ctx ctx;

	int server_fd = http_server_init(&ctx);

	zassert_true(server_fd >= 0, "Failed to create server socket");

	server_thread_id =
		k_thread_create(&server_thread, server_stack, STACK_SIZE, server_thread_fn, &ctx,
				NULL, NULL, K_PRIO_PREEMPT(8), 0, K_NO_WAIT);

	r = k_sem_take(&server_sem, K_MSEC(TIMEOUT));
	zassert_equal(0, r, "failed to synchronize with server thread (%d)", r);

	k_thread_name_set(k_current_get(), "client");

	r = socket(AF_INET, SOCK_STREAM, proto);
	zassert_not_equal(r, -1, "failed to create client socket (%d)", errno);
	client_fd = r;

	sa.sin_family = AF_INET;
	sa.sin_port = htons(SERVER_PORT);

	r = inet_pton(AF_INET, MY_IPV4_ADDR, &sa.sin_addr.s_addr);
	zassert_not_equal(-1, r, "inet_pton() failed (%d)", errno);
	zassert_not_equal(0, r, "%s is not a valid IPv4 address", MY_IPV4_ADDR);
	zassert_equal(1, r, "inet_pton() failed to convert %s", MY_IPV4_ADDR);

	memset(addrstr, '\0', sizeof(addrstr));
	addrstrp = (char *)inet_ntop(AF_INET, &sa.sin_addr, addrstr, sizeof(addrstr));
	zassert_not_equal(addrstrp, NULL, "inet_ntop() failed (%d)", errno);

	r = connect(client_fd, (struct sockaddr *)&sa, sizeof(sa));
	zassert_not_equal(r, -1, "failed to connect (%d)", errno);

	if (test_support == SUPPORT_BACKWARD_COMPATIBILITY) {

		char *http1_request = "GET / HTTP/1.1\r\n"
				      "Host: 127.0.0.1:8080\r\n"
				      "User-Agent: curl/7.68.0\r\n"
				      "Accept: */*\r\n"
				      "Accept-Encoding: deflate, gzip, br\r\n"
				      "\r\n";

		r = send(client_fd, http1_request, strlen(http1_request), 0);
		zassert_not_equal(r, -1, "send() failed (%d)", errno);

		char expected_response[] = "HTTP/1.1 200 OK\r\n"
					   "Content-Type: text/html\r\n"
					   "Content-Encoding: gzip\r\n"
					   "Content-Length: 14\r\n"
					   "\r\n";

		memset(addrstr, 0, sizeof(addrstr));
		r = recv(client_fd, addrstr, sizeof(addrstr), 0);
		zassert_not_equal(r, -1, "recv() failed (%d)", errno);

		zassert_equal(strcmp(addrstr, expected_response), 0,
			      "Received data doesn't match expected response");

	} else if (test_support == SUPPORT_HTTP_SERVER_UPGRADE) {

		r = send(client_fd, Frame, sizeof(Frame), 0);
		zassert_not_equal(r, -1, "send() failed (%d)", errno);

		memset(addrstr, 0, sizeof(addrstr));
		r = recv(client_fd, addrstr, sizeof(addrstr), 0);
		zassert_not_equal(r, -1, "recv() failed (%d)", errno);

		uint8_t type = addrstr[3];

		zassert_true(type == 0x4, "Expected a SETTINGS frame");
	}

	r = close(client_fd);
	zassert_not_equal(-1, r, "close() failed on the client fd (%d)", errno);

	http_server_stop(&ctx);

	r = k_thread_join(&server_thread, K_FOREVER);
	zassert_equal(0, r, "k_thread_join() failed (%d)", r);

	r = close(server_fd);
	zassert_not_equal(-1, r, "close() failed on the server fd (%d)", errno);
}

ZTEST(server_function_tests, test_http_upgrade)
{
	test_common(SUPPORT_HTTP_SERVER_UPGRADE);
}

ZTEST(server_function_tests, test_backward_compatibility)
{
	test_common(SUPPORT_BACKWARD_COMPATIBILITY);
}

ZTEST(server_function_tests, test_http_support_ipv6)
{
	struct http_server_ctx ctx;

	int server_fd = http_server_init(&ctx);

	/* Check that the function returned a valid file descriptor */
	zassert_true(server_fd >= 0, "Failed to initialize HTTP/2 server with IPv6");

	close(server_fd);
}

ZTEST(server_function_tests, test_http_support_ipv4)
{
	struct http_server_ctx ctx;

	int server_fd = http_server_init(&ctx);

	/* Check that the function returned a valid file descriptor */
	zassert_true(server_fd >= 0, "Failed to initialize HTTP/2 server with IPv4");

	close(server_fd);
}

static void server_thread_stp_fn(void *arg0, void *arg1, void *arg2)
{
	struct http_server_ctx *ctx = (struct http_server_ctx *)arg0;

	k_sem_give(&server_sem);

	int program_status = http_server_start(ctx);

	zassert_equal(program_status, 0, "The server didn't shut down successfully");
}

ZTEST(server_function_tests, test_http_server_stop)
{
	int r;

	k_sem_init(&server_sem, 0, 1);

	struct http_server_ctx ctx;

	int server_fd = http_server_init(&ctx);

	zassert_true(server_fd >= 0, "Failed to create server socket");

	k_thread_create(&server_thread, server_stack, STACK_SIZE, server_thread_stp_fn, &ctx, NULL,
			NULL, K_PRIO_PREEMPT(8), 0, K_NO_WAIT);
	k_sem_take(&server_sem, K_MSEC(TIMEOUT));

	http_server_stop(&ctx);

	r = k_thread_join(&server_thread, K_FOREVER);
	zassert_equal(0, r, "k_thread_join() failed (%d)", r);

	r = close(server_fd);
	zassert_not_equal(-1, r, "close() failed on the server fd (%d)", errno);
}

ZTEST(server_function_tests, test_http_server_init)
{
	struct http_server_ctx ctx;

	int server_fd = http_server_init(&ctx);

	/* Check that the function returned a valid file descriptor */
	zassert_true(server_fd >= 0, "Failed to initiate server socket");

	close(server_fd);
}

ZTEST(server_function_tests, test_get_frame_type_name)
{
	zassert_equal(strcmp(get_frame_type_name(HTTP_SERVER_DATA_FRAME), "DATA"), 0,
		      "Unexpected frame type");
	zassert_equal(strcmp(get_frame_type_name(HTTP_SERVER_HEADERS_FRAME), "HEADERS"), 0,
		      "Unexpected frame type");
	zassert_equal(strcmp(get_frame_type_name(HTTP_SERVER_PRIORITY_FRAME), "PRIORITY"), 0,
		      "Unexpected frame type");
	zassert_equal(strcmp(get_frame_type_name(HTTP_SERVER_RST_STREAM_FRAME), "RST_STREAM"), 0,
		      "Unexpected frame type");
	zassert_equal(strcmp(get_frame_type_name(HTTP_SERVER_SETTINGS_FRAME), "SETTINGS"), 0,
		      "Unexpected frame type");
	zassert_equal(strcmp(get_frame_type_name(HTTP_SERVER_PUSH_PROMISE_FRAME), "PUSH_PROMISE"),
		      0, "Unexpected frame type");
	zassert_equal(strcmp(get_frame_type_name(HTTP_SERVER_PING_FRAME), "PING"), 0,
		      "Unexpected frame type");
	zassert_equal(strcmp(get_frame_type_name(HTTP_SERVER_GOAWAY_FRAME), "GOAWAY"), 0,
		      "Unexpected frame type");
	zassert_equal(strcmp(get_frame_type_name(HTTP_SERVER_WINDOW_UPDATE_FRAME), "WINDOW_UPDATE"),
		      0, "Unexpected frame type");
	zassert_equal(strcmp(get_frame_type_name(HTTP_SERVER_CONTINUATION_FRAME), "CONTINUATION"),
		      0, "Unexpected frame type");
}

ZTEST(server_function_tests, test_parse_http_frames)
{
	struct http_client_ctx ctx_client1;
	struct http_client_ctx ctx_client2;
	struct http_frame *frame;

	unsigned char buffer1[] = {0x00, 0x00, 0x0c, 0x04, 0x00, 0x00, 0x00, 0x00,
				   0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x64, 0x00,
				   0x04, 0x00, 0x00, 0xff, 0xff, 0x00};
	unsigned char buffer2[] = {0x00, 0x00, 0x21, 0x01, 0x05, 0x00, 0x00, 0x00, 0x01, 0x82, 0x84,
				   0x86, 0x41, 0x8a, 0x0b, 0xe2, 0x5c, 0x0b, 0x89, 0x70, 0xdc, 0x78,
				   0x0f, 0x03, 0x53, 0x03, 0x2a, 0x2f, 0x2a, 0x90, 0x7a, 0x8a, 0xaa,
				   0x69, 0xd2, 0x9a, 0xc4, 0xc0, 0x57, 0x68, 0x0b, 0x83};

	memcpy(ctx_client1.buffer, buffer1, sizeof(buffer1));
	memcpy(ctx_client2.buffer, buffer2, sizeof(buffer2));

	ctx_client1.offset = ARRAY_SIZE(buffer1);

	ctx_client2.offset = ARRAY_SIZE(buffer2);

	/* Test: Buffer with the first frame */
	int parser1 = parse_http_frame_header(&ctx_client1);

	zassert_equal(parser1, 1, "Failed to parse the first frame");

	frame = &ctx_client1.current_frame;

	/* Validate frame details for the 1st frame */
	zassert_equal(frame->length, 0x0C, "Expected length for the 1st frame doesn't match");
	zassert_equal(frame->type, 0x04, "Expected type for the 1st frame doesn't match");
	zassert_equal(frame->flags, 0x00, "Expected flags for the 1st frame doesn't match");
	zassert_equal(frame->stream_identifier, 0x00,
		      "Expected stream_identifier for the 1st frame doesn't match");

	/* Test: Buffer with the second frame */
	int parser2 = parse_http_frame_header(&ctx_client2);

	zassert_equal(parser2, 1, "Failed to parse the second frame");

	frame = &ctx_client2.current_frame;

	/* Validate frame details for the 2nd frame */
	zassert_equal(frame->length, 0x21, "Expected length for the 2nd frame doesn't match");
	zassert_equal(frame->type, 0x01, "Expected type for the 2nd frame doesn't match");
	zassert_equal(frame->flags, 0x05, "Expected flags for the 2nd frame doesn't match");
	zassert_equal(frame->stream_identifier, 0x01,
		      "Expected stream_identifier for the 2nd frame doesn't match");
}

ZTEST_SUITE(server_function_tests, NULL, NULL, NULL, NULL, NULL);
