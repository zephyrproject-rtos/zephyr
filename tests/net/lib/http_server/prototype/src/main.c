/*
 * Copyright (c) 2023, Emna Rekik
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/net/socket.h>
#include <string.h>
#include <zephyr/net/http/server.h>

ZTEST_SUITE(server_function_tests, NULL, NULL, NULL, NULL, NULL);

ZTEST(server_function_tests, test_http2_server_init)
{
	struct http2_server_config config;
	struct http2_server_ctx ctx;

	config.port = 8080;
	config.address_family = AF_INET;

	int server_fd = http2_server_init(&ctx, &config);

	/* Check that the function returned a valid file descriptor */
	zassert_true(server_fd >= 0, "Failed to create server socket");

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
