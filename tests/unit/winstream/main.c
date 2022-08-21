/*
 * Copyright (c) 2021 Intel Corporation
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/ztest.h>
#include <zephyr/sys/winstream.h>

/* This, uh, seems to be the standard way to unit test library code.
 * Or so I gather from tests/unit/rbtree ...
 */
#include "../../../lib/os/winstream.c"

#define BUFLEN 64

const char *msg = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
char wsmem[BUFLEN + 1]; /* Extra 1 to have a null for easier debugging */

ZTEST(winstream, test_winstream)
{
	struct sys_winstream *ws = sys_winstream_init(wsmem, BUFLEN);

	/* Write one byte */
	sys_winstream_write(ws, "a", 1);

	uint32_t seq = 0;
	char c;

	/* Read the byte back */
	uint32_t bytes = sys_winstream_read(ws, &seq, &c, 1);

	zassert_true(bytes == 1, "");
	zassert_true(seq == 1, "");
	zassert_true(c == 'a', "");

	/* Read from an empty buffer */
	bytes = sys_winstream_read(ws, &seq, &c, 1);
	zassert_true(bytes == 0, "");
	zassert_true(seq == 1, "");

	/* Write an overflowing string */
	sys_winstream_write(ws, msg, strlen(msg));
	zassert_true(ws->seq == 1 + strlen(msg), "");
	zassert_true(ws->start == 1, "");
	zassert_true(ws->end == 0, "");

	/* Read after underflow, verify empty string comes back with the
	 * correct sequence number
	 */
	char readback[BUFLEN + 1];

	memset(readback, 0, sizeof(readback));
	bytes = sys_winstream_read(ws, &seq, readback, sizeof(readback));
	zassert_true(seq == ws->seq, "");
	zassert_true(bytes == 0, "");

	/* Read back from empty buffer */
	uint32_t seq0 = seq;

	bytes = sys_winstream_read(ws, &seq, readback, sizeof(readback));
	zassert_true(seq == seq0, "");
	zassert_true(bytes == 0, "");

	/* Write a "short-enough" string that fits in before the wrap,
	 * then read it out
	 */
	seq0 = seq;
	sys_winstream_write(ws, msg, ws->len / 2);
	bytes = sys_winstream_read(ws, &seq, readback, sizeof(readback));
	zassert_true(bytes == ws->len / 2, "");
	zassert_true(seq == seq0 + ws->len / 2, "");
	zassert_true(strncmp(readback, msg, ws->len / 2) == 0, "");

	/* Do it again, this time it will need to wrap around the buffer */
	memset(readback, 0, sizeof(readback));
	seq0 = seq;
	sys_winstream_write(ws, msg, ws->len / 2);
	bytes = sys_winstream_read(ws, &seq, readback, sizeof(readback));
	zassert_true(bytes == ws->len / 2, "");
	zassert_true(seq == seq0 + ws->len / 2, "");
	zassert_true(strncmp(readback, msg, ws->len / 2) == 0, "");

	/* Finally loop with a relatively prime (actually prime prime)
	 * buffer size to stress for edges.
	 */
	int n = 13;
	char msg2[13];

	for (int i = 0; i < (n + 1) * (ws->len + 1); i++) {
		memset(msg2, 'A' + (i % 26), n);
		seq0 = seq;
		memset(readback, 0, sizeof(readback));
		sys_winstream_write(ws, msg2, n);
		bytes = sys_winstream_read(ws, &seq, readback, sizeof(readback));
		zassert_true(bytes == n, "");
		zassert_true(seq == seq0 + n, "");
		zassert_true(strncmp(readback, msg2, n) == 0, "");
	}
}

ZTEST_SUITE(winstream, NULL, NULL, NULL, NULL, NULL);
