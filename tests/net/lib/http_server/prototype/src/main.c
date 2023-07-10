/*
 * Copyright (c) 2023, Emna Rekik
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/net/socket.h>
#include <string.h>
#include <zephyr/net/http/server.h>
#include "server_functions.h"

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

ZTEST_SUITE(server_function_tests, NULL, NULL, NULL, NULL, NULL);

ZTEST(server_function_tests, test_gen_gz_inc_file)
{
	const char *data;
	size_t len;

	data = inc_file;
	len = sizeof(inc_file);
	len = len - 2;

	zassert_equal(len, sizeof(compressed_inc_file), "Invalid compressed file size");

	int result = memcmp(data, compressed_inc_file, len);

	zassert_equal(result, 0, "inc_file and compressed_inc_file contents are not identical.");
}

ZTEST(server_function_tests, test_http2_support_ipv6)
{
	struct http2_server_config config;
	struct http2_server_ctx ctx;

	config.port = 8080;
	config.address_family = AF_INET6;

	int server_fd = http2_server_init(&ctx, &config);

	/* Check that the function returned a valid file descriptor */
	zassert_true(server_fd >= 0, "Failed to initialize HTTP/2 server with IPv6");

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
	zassert_true(server_fd >= 0, "Failed to initialize HTTP/2 server with IPv4");

	close(server_fd);
}

ZTEST(server_function_tests, test_http2_server_stop)
{
	int sem_count = http2_server_stop(NULL, 0, NULL);

	zassert_equal(sem_count, 1, "Semaphore should have one token after 'quit' command");
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

ZTEST(server_function_tests, test_parse_http2_frames)
{
	unsigned char buffer[] = {0x00, 0x00, 0x0c, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03,
				  0x00, 0x00, 0x00, 0x64, 0x00, 0x04, 0x00, 0x00, 0xff, 0xff, 0x00,
				  0x00, 0x21, 0x01, 0x05, 0x00, 0x00, 0x00, 0x01, 0x82, 0x84, 0x86,
				  0x41, 0x8a, 0x0b, 0xe2, 0x5c, 0x0b, 0x89, 0x70, 0xdc, 0x78, 0x0f,
				  0x03, 0x53, 0x03, 0x2a, 0x2f, 0x2a, 0x90, 0x7a, 0x8a, 0xaa, 0x69,
				  0xd2, 0x9a, 0xc4, 0xc0, 0x57, 0x68, 0x0b, 0x83};
	struct http2_frame frames[2];
	unsigned int buffer_len = ARRAY_SIZE(buffer);
	unsigned int frame_count;

	/* Test: Buffer with exactly two frames */
	frame_count = parse_http2_frames(buffer, buffer_len, frames);
	zassert_equal(frame_count, 2, "Expected 2 frames for the given payload");

	/* Validate frame details for the 1st frame */
	zassert_equal(frames[0].length, 0x0C, "Expected length for the 1st frame doesn't match");
	zassert_equal(frames[0].type, 0x04, "Expected type for the 1st frame doesn't match");
	zassert_equal(frames[0].flags, 0x00, "Expected flags for the 1st frame doesn't match");
	zassert_equal(frames[0].stream_identifier, 0x00,
		      "Expected stream_identifier for the 1st frame doesn't match");

	/* Validate frame details for the 2nd frame */
	zassert_equal(frames[1].length, 0x21, "Expected length for the 2nd frame doesn't match");
	zassert_equal(frames[1].type, 0x01, "Expected type for the 2nd frame doesn't match");
	zassert_equal(frames[1].flags, 0x05, "Expected flags for the 2nd frame doesn't match");
	zassert_equal(frames[1].stream_identifier, 0x01,
		      "Expected stream_identifier for the 2nd frame doesn't match");
}
