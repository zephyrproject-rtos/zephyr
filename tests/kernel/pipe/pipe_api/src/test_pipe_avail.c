/*
 * Copyright (c) 2020 Friedt Professional Engineering Services, Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief Tests for the Pipe read / write availability
 * @ingroup kernel_pipe_tests
 * @{
 */

#include <ztest.h>

static ZTEST_DMEM unsigned char __aligned(4) data[] = "abcdefgh";
static struct k_pipe pipe = {
	.buffer = data,
	.size = sizeof(data) - 1 /* '\0' */,
};

static struct k_pipe bufferless;

static struct k_pipe bufferless1 = {
	.buffer = data,
	.size = 0,
};

/**
 * @brief Pipes with no buffer or size 0 should return 0 bytes available
 *
 * Pipes can be created to be bufferless (i.e. @ref k_pipe.buffer is `NULL`
 * or @ref k_pipe.size is 0).
 *
 * If either of those conditions is true, then @ref k_pipe_read_avail and
 * @ref k_pipe_write_avail should return 0.
 *
 * @note
 * A distinction can be made between buffered and bufferless pipes in that
 * @ref k_pipe_read_avail and @ref k_pipe_write_avail will never
 * simultaneously return 0 for a buffered pipe, but they will both return 0
 * for an unbuffered pipe.
 */
ZTEST(pipe_api, test_pipe_avail_no_buffer)
{
	size_t r_avail;
	size_t w_avail;

	r_avail = k_pipe_read_avail(&bufferless);
	zassert_equal(r_avail, 0, "read: expected: 0 actual: %u", r_avail);

	w_avail = k_pipe_write_avail(&bufferless);
	zassert_equal(w_avail, 0, "write: expected: 0 actual: %u", w_avail);

	r_avail = k_pipe_read_avail(&bufferless1);
	zassert_equal(r_avail, 0, "read: expected: 0 actual: %u", r_avail);

	w_avail = k_pipe_write_avail(&bufferless1);
	zassert_equal(w_avail, 0, "write: expected: 0 actual: %u", w_avail);
}

/**
 * @brief Test available read / write space for r < w
 *
 * This test case is for buffered @ref k_pipe objects and covers the case
 * where @ref k_pipe.read_index is less than @ref k_pipe.write_index.
 *
 * In this case, @ref k_pipe.bytes_used is not relevant.
 *
 *      r     w
 *     |a|b|c|d|e|f|g|h|
 *     |0|1|2|3|4|5|6|7|
 *
 * As shown above, the pipe will be able to read 3 bytes without blocking
 * and write 5 bytes without blocking.
 *
 * Thus
 *     r_avail = w - r = 3
 *     would read: a b c d
 *
 *     w_avail = N - (w - r) = 5
 *     would overwrite: e f g h
 */
ZTEST(pipe_api, test_pipe_avail_r_lt_w)
{
	size_t r_avail;
	size_t w_avail;

	pipe.read_index = 0;
	pipe.write_index = 3;
	/* pipe.bytes_used is irrelevant */

	r_avail = k_pipe_read_avail(&pipe);
	zassert_equal(r_avail, 3, "read: expected: 3 actual: %u", r_avail);

	w_avail = k_pipe_write_avail(&pipe);
	zassert_equal(w_avail, 5, "write: expected: 5 actual: %u", w_avail);
}

/**
 * @brief Test available read / write space for w < r
 *
 * This test case is for buffered @ref k_pipe objects and covers the case
 * where @ref k_pipe.write_index is less than @ref k_pipe.read_index.
 *
 * In this case, @ref k_pipe.bytes_used is not relevant.
 *
 *      w     r
 *     |a|b|c|d|e|f|g|h|
 *     |0|1|2|3|4|5|6|7|
 *
 *
 * As shown above, the pipe will fbe able to read 5 bytes without blocking
 * and write 3 bytes without blocking.
 *
 * Thus
 *     r_avail = N - (r - w) = 5
 *     would read: e f g h
 *
 *     w_avail = r - w = 3
 *     would overwrite: a b c d
 */
ZTEST(pipe_api, test_pipe_avail_w_lt_r)
{
	size_t r_avail;
	size_t w_avail;

	pipe.read_index = 3;
	pipe.write_index = 0;
	/* pipe.bytes_used is irrelevant */

	r_avail = k_pipe_read_avail(&pipe);
	zassert_equal(r_avail, 5, "read: expected: 4 actual: %u", r_avail);

	w_avail = k_pipe_write_avail(&pipe);
	zassert_equal(w_avail, 3, "write: expected: 4 actual: %u", w_avail);
}

/**
 * @brief Test available read / write space for `r == w` and an empty buffer
 *
 * This test case is for buffered @ref k_pipe objects and covers the case
 * where @ref k_pipe.read_index is equal to @ref k_pipe.write_index and
 * @ref k_pipe.bytes_used is zero.
 *
 * In this case, @ref k_pipe.bytes_used is relevant because the read and
 * write indices are equal.
 *
 *            r
 *            w
 *     |a|b|c|d|e|f|g|h|
 *     |0|1|2|3|4|5|6|7|
 *
 * Regardless of whether the buffer is full or empty, the following holds:
 *
 *     r_avail = bytes_used
 *     w_avail = N - bytes_used
 *
 * Thus:
 *     r_avail = 0
 *     would read:
 *
 *     w_avail = N - 0 = 8
 *     would overwrite: e f g h a b c d
 */
ZTEST(pipe_api, test_pipe_avail_r_eq_w_empty)
{
	size_t r_avail;
	size_t w_avail;

	pipe.read_index = 4;
	pipe.write_index = 4;
	pipe.bytes_used = 0;

	r_avail = k_pipe_read_avail(&pipe);
	zassert_equal(r_avail, 0, "read: expected: 0 actual: %u", r_avail);

	w_avail = k_pipe_write_avail(&pipe);
	zassert_equal(w_avail, 8, "write: expected: 8 actual: %u", w_avail);
}

/**
 * @brief Test available read / write space for `r == w` and a full buffer
 *
 * This test case is for buffered @ref k_pipe objects and covers the case
 * where @ref k_pipe.read_index is equal to @ref k_pipe.write_index and
 * @ref k_pipe.bytes_used is equal to @ref k_pipe.size.
 *
 * In this case, @ref k_pipe.bytes_used is relevant because the read and
 * write indices are equal.
 *
 *            r
 *            w
 *     |a|b|c|d|e|f|g|h|
 *     |0|1|2|3|4|5|6|7|
 *
 * Regardless of whether the buffer is full or empty, the following holds:
 *
 *     r_avail = bytes_used
 *     w_avail = N - bytes_used
 *
 * Thus
 *     r_avail = N = 8
 *     would read: e f g h a b c d
 *
 *     w_avail = N - 8 = 0
 *     would overwrite:
 */
ZTEST(pipe_api, test_pipe_avail_r_eq_w_full)
{
	size_t r_avail;
	size_t w_avail;

	pipe.read_index = 4;
	pipe.write_index = 4;
	pipe.bytes_used = pipe.size;

	r_avail = k_pipe_read_avail(&pipe);
	zassert_equal(r_avail, 8, "read: expected: 8 actual: %u", r_avail);

	w_avail = k_pipe_write_avail(&pipe);
	zassert_equal(w_avail, 0, "write: expected: 0 actual: %u", w_avail);
}

/**
 * @}
 */
