/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-FileCopyrightText: Copyright (c) 2026 DevItWise
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stddef.h>
#include <string.h>

#include <zephyr/net/tftp.h>
#include <zephyr/ztest.h>

#include "tftp_client.h"

#define REQUEST_BUF_PADDING 256

#define DEFAULT_MODE_LEN 5

static void fill(char *dst, size_t len, char c)
{
	memset(dst, c, len);
	dst[len] = '\0';
}

ZTEST(tftp_client, test_make_request_filename_one_under_budget)
{
	uint8_t buf[TFTPC_MAX_BUF_SIZE + REQUEST_BUF_PADDING];
	char remote_file[TFTP_MAX_FILENAME_SIZE];
	size_t req_size;

	fill(remote_file, TFTP_MAX_FILENAME_SIZE - 1, 'a');

	req_size = make_request(buf, READ_REQUEST, remote_file, NULL);

	zassert_equal(req_size, 2 + (TFTP_MAX_FILENAME_SIZE - 1) + 1 + DEFAULT_MODE_LEN + 1,
		      "unexpected request size %zu for a filename one byte under budget", req_size);
}

ZTEST(tftp_client, test_make_request_filename_at_budget)
{
	uint8_t buf[TFTPC_MAX_BUF_SIZE + REQUEST_BUF_PADDING];
	char remote_file[TFTP_MAX_FILENAME_SIZE + 1];
	size_t req_size;

	fill(remote_file, TFTP_MAX_FILENAME_SIZE, 'a');

	req_size = make_request(buf, READ_REQUEST, remote_file, NULL);

	zassert_equal(req_size, 2 + TFTP_MAX_FILENAME_SIZE + 1 + DEFAULT_MODE_LEN + 1,
		      "unexpected request size %zu for a filename exactly at budget", req_size);
}

ZTEST(tftp_client, test_make_request_filename_one_over_budget)
{
	uint8_t buf[TFTPC_MAX_BUF_SIZE + REQUEST_BUF_PADDING];
	char remote_file[TFTP_MAX_FILENAME_SIZE + 2];
	size_t req_size;

	fill(remote_file, TFTP_MAX_FILENAME_SIZE + 1, 'a');

	req_size = make_request(buf, READ_REQUEST, remote_file, NULL);

	zassert_equal(req_size, 2 + TFTP_MAX_FILENAME_SIZE + 1 + DEFAULT_MODE_LEN + 1,
		      "unexpected request size %zu for a filename one byte over budget", req_size);
}

ZTEST(tftp_client, test_make_request_mode_one_under_budget)
{
	uint8_t buf[TFTPC_MAX_BUF_SIZE + REQUEST_BUF_PADDING];
	const char *remote_file = "file.bin";
	char mode[TFTP_MAX_MODE_SIZE];
	size_t req_size;

	fill(mode, TFTP_MAX_MODE_SIZE - 1, 'b');

	req_size = make_request(buf, READ_REQUEST, remote_file, mode);

	zassert_equal(req_size, 2 + strlen(remote_file) + 1 + (TFTP_MAX_MODE_SIZE - 1) + 1,
		      "unexpected request size %zu for a mode one byte under budget", req_size);
}

ZTEST(tftp_client, test_make_request_mode_at_budget)
{
	uint8_t buf[TFTPC_MAX_BUF_SIZE + REQUEST_BUF_PADDING];
	const char *remote_file = "file.bin";
	char mode[TFTP_MAX_MODE_SIZE + 1];
	size_t req_size;

	fill(mode, TFTP_MAX_MODE_SIZE, 'b');

	req_size = make_request(buf, READ_REQUEST, remote_file, mode);

	zassert_equal(req_size, 2 + strlen(remote_file) + 1 + TFTP_MAX_MODE_SIZE + 1,
		      "unexpected request size %zu for a mode exactly at budget", req_size);
}

ZTEST(tftp_client, test_make_request_mode_one_over_budget)
{
	uint8_t buf[TFTPC_MAX_BUF_SIZE + REQUEST_BUF_PADDING];
	const char *remote_file = "file.bin";
	char mode[TFTP_MAX_MODE_SIZE + 2];
	size_t req_size;

	fill(mode, TFTP_MAX_MODE_SIZE + 1, 'b');

	req_size = make_request(buf, READ_REQUEST, remote_file, mode);

	zassert_equal(req_size, 2 + strlen(remote_file) + 1 + TFTP_MAX_MODE_SIZE + 1,
		      "unexpected request size %zu for a mode one byte over budget", req_size);
}

ZTEST(tftp_client, test_make_request_filename_and_mode_at_budget)
{
	uint8_t buf[TFTPC_MAX_BUF_SIZE + REQUEST_BUF_PADDING];
	char remote_file[TFTP_MAX_FILENAME_SIZE + 1];
	char mode[TFTP_MAX_MODE_SIZE + 1];
	size_t req_size;

	fill(remote_file, TFTP_MAX_FILENAME_SIZE, 'a');
	fill(mode, TFTP_MAX_MODE_SIZE, 'b');

	req_size = make_request(buf, READ_REQUEST, remote_file, mode);

	zassert_equal(req_size, TFTPC_MAX_BUF_SIZE,
		      "unexpected request size %zu with both filename and mode at budget",
		      req_size);
}

ZTEST(tftp_client, test_make_request_short_filename_unaffected)
{
	uint8_t buf[TFTPC_MAX_BUF_SIZE];
	const char *remote_file = "boot.bin";
	size_t req_size;

	req_size = make_request(buf, READ_REQUEST, remote_file, NULL);

	zassert_equal(req_size, 2 + strlen(remote_file) + 1 + DEFAULT_MODE_LEN + 1,
		      "unexpected request size %zu for a short filename", req_size);
}
