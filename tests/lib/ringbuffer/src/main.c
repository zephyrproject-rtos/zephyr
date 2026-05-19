/*
 * Copyright (c) 2016 Intel Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <zephyr/ztest.h>
#include <zephyr/irq_offload.h>
#include <zephyr/sys/ring_buffer.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(test, LOG_LEVEL_DBG);

/**
 * @defgroup lib_ringbuffer_tests Ringbuffer
 * @ingroup all_tests
 * @{
 * @}
 */

#define RINGBUFFER_SIZE 5

RING_BUF_DECLARE(ringbuf_raw, RINGBUFFER_SIZE);

/**
 * @brief verify that ringbuffer can be placed in any user-controlled memory
 *
 * @details
 * Test Objective:
 * - define and initialize a ring buffer by macro RING_BUF_DECLARE,
 * then verify data is passed between ring buffer and array
 *
 * Testing techniques:
 * - Interface testing
 * - Dynamic analysis and testing
 * - Structural test coverage(entry points,statements,branches)
 *
 * Prerequisite Conditions:
 * - Define and initialize a ringbuffer by RING_BUF_DECLARE
 *
 * Input Specifications:
 * - N/A
 *
 * Test Procedure:
 * -# Define two arrays(inbuf,outbuf) and initialize inbuf
 * -# Put and get data with "for loop"
 * -# Check if data size pushed is equal to what are gotten.
 * -# Then initialize the output buffer
 * -# Put data with different size to check if data size
 * pushed is equal to what are gotten.
 *
 * Expected Test Result:
 * - data items pushed shall be equal to what are gotten.
 *
 * Pass/Fail Criteria:
 * - Success if test result of step 4,5 is passed. Otherwise, failure.
 *
 * Assumptions and Constraints:
 * - N/A
 *
 * @ingroup lib_ringbuffer_tests
 *
 * @see ring_buf_put, ring_buf_get
 */
ZTEST(ringbuffer_api, test_ringbuffer_raw)
{
	int i;
	uint8_t inbuf[RINGBUFFER_SIZE];
	uint8_t outbuf[RINGBUFFER_SIZE];
	size_t in_size;
	size_t out_size;

	/* Initialize test buffer. */
	for (i = 0; i < RINGBUFFER_SIZE; i++) {
		inbuf[i] = i;
	}

	for (i = 0; i < 10; i++) {
		memset(outbuf, 0, sizeof(outbuf));
		in_size = ring_buf_put(&ringbuf_raw, inbuf,
					       RINGBUFFER_SIZE - 2);
		out_size = ring_buf_get(&ringbuf_raw, outbuf,
						RINGBUFFER_SIZE - 2);

		zassert_true(in_size == RINGBUFFER_SIZE - 2);
		zassert_true(in_size == out_size);
		zassert_true(memcmp(inbuf, outbuf, RINGBUFFER_SIZE - 2) == 0);
	}

	memset(outbuf, 0, sizeof(outbuf));
	in_size = ring_buf_put(&ringbuf_raw, inbuf,
				       RINGBUFFER_SIZE);
	zassert_equal(in_size, RINGBUFFER_SIZE);

	in_size = ring_buf_put(&ringbuf_raw, inbuf,
				       1);
	zassert_equal(in_size, 0);

	out_size = ring_buf_get(&ringbuf_raw, outbuf,
					RINGBUFFER_SIZE);

	zassert_true(out_size == RINGBUFFER_SIZE);

	out_size = ring_buf_get(&ringbuf_raw, outbuf,
					RINGBUFFER_SIZE + 1);
	zassert_true(out_size == 0);
	zassert_true(ring_buf_is_empty(&ringbuf_raw));

	/* Validate that raw bytes can be discarded */
	in_size = ring_buf_put(&ringbuf_raw, inbuf,
				       RINGBUFFER_SIZE);
	zassert_equal(in_size, RINGBUFFER_SIZE);

	out_size = ring_buf_get(&ringbuf_raw, NULL,
					RINGBUFFER_SIZE);
	zassert_true(out_size == RINGBUFFER_SIZE);

	out_size = ring_buf_get(&ringbuf_raw, NULL,
					RINGBUFFER_SIZE + 1);
	zassert_true(out_size == 0);
	zassert_true(ring_buf_is_empty(&ringbuf_raw));
}

ZTEST(ringbuffer_api, test_capacity)
{
	uint32_t capacity;

	ring_buf_init(&ringbuf_raw, RINGBUFFER_SIZE + 1, ringbuf_raw.buffer);

	capacity = ring_buf_capacity_get(&ringbuf_raw);
	zassert_equal(RINGBUFFER_SIZE, capacity,
			"Unexpected capacity");
}

ZTEST(ringbuffer_api, test_ringbuffer_size)
{
	LOG_DBG("target:%s, sizeof(struct ring_buf) = %zu", CONFIG_BOARD, sizeof(struct ring_buf));
}

ZTEST(ringbuffer_api, test_size)
{
	uint32_t size;
	static uint8_t buf[RINGBUFFER_SIZE];

	ring_buf_init(&ringbuf_raw, sizeof(buf) + 1, ringbuf_raw.buffer);

	/* Test 0 */
	size = ring_buf_size_get(&ringbuf_raw);
	zassert_equal(0, size, "wrong size: exp: %u act: %u", 0, size);

	/* Test 1 */
	ring_buf_put(&ringbuf_raw, "x", 1);
	size = ring_buf_size_get(&ringbuf_raw);
	zassert_equal(1, size, "wrong size: exp: %u act: %u", 1, size);

	/* Test N */
	ring_buf_reset(&ringbuf_raw);
	ring_buf_put(&ringbuf_raw, buf, sizeof(buf));
	size = ring_buf_size_get(&ringbuf_raw);
	zassert_equal(sizeof(buf), size, "wrong size: exp: %zu: actual: %u", sizeof(buf), size);

	/* Test N - 2 with wrap-around */
	ring_buf_put(&ringbuf_raw, buf, sizeof(buf));
	ring_buf_get(&ringbuf_raw, NULL, 3);
	ring_buf_put(&ringbuf_raw, "x", 1);

	size = ring_buf_size_get(&ringbuf_raw);
	zassert_equal(sizeof(buf) - 2, size, "wrong size: exp: %zu: actual: %u", sizeof(buf) - 2,
		      size);
}

ZTEST(ringbuffer_api, test_peek)
{
	uint32_t size;
	uint8_t byte = 0x42;
	static uint8_t buf[RINGBUFFER_SIZE];

	ring_buf_init(&ringbuf_raw, sizeof(buf) + 1, ringbuf_raw.buffer);

	/* Test 0 */
	size = ring_buf_peek(&ringbuf_raw, (uint8_t *)0x1, 42424242);
	zassert_equal(0, size, "wrong peek size: exp: %u: actual: %u", 0, size);

	/* Test 1 */
	ring_buf_put(&ringbuf_raw, "*", 1);
	size = ring_buf_peek(&ringbuf_raw, &byte, 1);
	zassert_equal(1, size, "wrong peek size: exp: %u: actual: %u", 1, size);
	zassert_equal('*', byte, "wrong buffer contents: exp: %u: actual: %u", '*', byte);
	size = ring_buf_size_get(&ringbuf_raw);
	zassert_equal(1, size, "wrong buffer size: exp: %u: actual: %u", 1, size);

	/* Test N */
	ring_buf_reset(&ringbuf_raw);
	for (size = 0; size < sizeof(buf); ++size) {
		buf[size] = 'A' + (size % ('Z' - 'A' + 1));
	}

	ring_buf_put(&ringbuf_raw, buf, sizeof(buf));
	memset(buf, '*', sizeof(buf)); /* fill with pattern */

	size = ring_buf_peek(&ringbuf_raw, buf, sizeof(buf));
	zassert_equal(sizeof(buf), size,
			"wrong peek size: exp: %zu: actual: %u", sizeof(buf), size);
	size = ring_buf_size_get(&ringbuf_raw);
	zassert_equal(sizeof(buf), size, "wrong buffer size: exp: %zu: actual: %u", sizeof(buf),
		      size);

	for (size = 0; size < sizeof(buf); ++size) {
		ringbuf_raw.buffer[size] = 'A' + (size % ('Z' - 'A' + 1));
	}

	zassert_equal(0, memcmp(buf, ringbuf_raw.buffer, sizeof(buf)),
		      "content validation failed");
}

ZTEST(ringbuffer_api, test_reset)
{
	uint8_t indata[] = {1, 2, 3, 4, 5};
	uint8_t outdata[RINGBUFFER_SIZE];
	uint8_t *outbuf;
	uint32_t len;
	uint32_t out_len;
	uint32_t granted;
	uint32_t space;

	ring_buf_init(&ringbuf_raw, RINGBUFFER_SIZE + 1, ringbuf_raw.buffer);

	len = 3;
	out_len = ring_buf_put(&ringbuf_raw, indata, len);
	zassert_equal(out_len, len);

	out_len = ring_buf_get(&ringbuf_raw, outdata, len);
	zassert_equal(out_len, len);

	space = ring_buf_space_get(&ringbuf_raw);
	zassert_equal(space, RINGBUFFER_SIZE);

	/* Even though ringbuffer is empty, full buffer cannot be allocated
	 * because internal pointers are not at the beginning.
	 */
	granted = ring_buf_put_ptr(&ringbuf_raw, &outbuf);
	zassert_false(granted == RINGBUFFER_SIZE);

	/* After reset full buffer can be allocated. */
	ring_buf_reset(&ringbuf_raw);
	granted = ring_buf_put_ptr(&ringbuf_raw, &outbuf);
	zassert_true(granted == RINGBUFFER_SIZE);
}

ZTEST(ringbuffer_api, test_ringbuffer_zero_copy)
{
	uint8_t *ref, *ref2;
	size_t read_sz;
	size_t write_sz;
	uint8_t buf[16 + 1];
	uint8_t input[12];
	struct ring_buf rb;

	for (uint8_t i = 0; i < sizeof(input); i++) {
		input[i] = i;
	}

	ring_buf_init(&rb, sizeof(buf), buf);

	ring_buf_put(&rb, input, sizeof(input));

	ref = NULL;
	read_sz = ring_buf_get_ptr(&rb, &ref);
	zassert_equal(read_sz, sizeof(input), "Unexpected read size");
	zassert_true(memcmp(ref, input, read_sz) == 0, "Data corrupted after get_ptr");
	zassert_equal(ring_buf_size_get(&rb), read_sz, "Size calculation incorrect after get_ptr");
	zassert_equal(ring_buf_capacity_get(&rb), sizeof(buf) - 1, "Capacity changed after get_ptr");
	zassert_equal(ring_buf_space_get(&rb), sizeof(buf) - 1 - read_sz,
			"Space calculation incorrect after get_ptr");

	zassert_equal(ring_buf_get_ptr(&rb, &ref2), read_sz,
			"Subsequent get_ptr calls returned different sizes");
	zassert_equal(ref, ref2, "Subsequent get_ptr calls returned different pointers");

	ring_buf_consume(&rb, read_sz);
	zassert_equal(ring_buf_size_get(&rb), 0, "Size not zero after consuming all data");
	zassert_equal(ring_buf_capacity_get(&rb), sizeof(buf) - 1, "Capacity changed after consume");
	zassert_equal(ring_buf_space_get(&rb), sizeof(buf) - 1,
			"Space not fully restored after consuming all data");
	zassert_equal(ring_buf_get_ptr(&rb, &ref), 0,
			"Data still available after consuming all data");

	write_sz = ring_buf_put_ptr(&rb, &ref);
	zassert_equal(write_sz, sizeof(buf) - sizeof(input),
			"Unexpected write size");
	zassert_equal(ring_buf_size_get(&rb), 0, "Change in data without commit");
	zassert_equal(ring_buf_capacity_get(&rb), sizeof(buf) - 1, "Capacity changed after put_ptr");
	zassert_equal(ring_buf_space_get(&rb), sizeof(buf) - 1, "Space changed after put_ptr");

	zassert_equal(ring_buf_put_ptr(&rb, &ref2), write_sz,
			"Subsequent put_ptr calls returned different sizes");
	zassert_equal(ref, ref2, "Subsequent put_ptr calls returned different pointers");

	memcpy(ref, &input[0], write_sz);
	ring_buf_commit(&rb, write_sz);
	zassert_equal(ring_buf_size_get(&rb), write_sz,
			"Size not updated after commit");

	zassert_equal(ring_buf_get_ptr(&rb, &ref), write_sz, "Data size incorrect after commit");
	zassert_true(memcmp(ref, &input[0], write_sz) == 0, "Data corrupted after commit");
	ring_buf_consume(&rb, write_sz);

	write_sz = ring_buf_put_ptr(&rb, &ref);
	zassert_equal(ref, buf, "Data pointer incorrect after buffer empty");
	zassert_equal(write_sz, sizeof(buf) - 1,
			"Unexpected write size after buffer empty");
	memcpy(ref, input, sizeof(buf) / 2);
	ring_buf_commit(&rb, sizeof(buf) / 2);

	zassert_equal(ring_buf_size_get(&rb), sizeof(buf) / 2,
			"Size not updated after partial commit");
	zassert_equal(ring_buf_space_get(&rb), sizeof(buf) / 2,
			"Space not updated after partial commit");
	zassert_equal(ring_buf_get_ptr(&rb, &ref), sizeof(buf) / 2,
			"Data size incorrect after partial commit");
	zassert_equal(ref, buf, "Data pointer incorrect after partial commit");
}


ZTEST(ringbuffer_api, test_ringbuffer_put_ptr)
{
	uint8_t *ref, *ref2;
	size_t write_sz, write_sz2;
	uint8_t buf[16 + 1];
	uint8_t tmp[16];
	uint8_t input[13];
	struct ring_buf rb;

	for (uint8_t i = 0; i < sizeof(input); i++) {
		input[i] = i;
	}

	ring_buf_init(&rb, sizeof(buf), buf);

	write_sz = ring_buf_put_ptr(&rb, &ref);
	zassert_equal(write_sz, sizeof(buf) - 1, "Unexpected write size");
	write_sz2 = ring_buf_put_ptr(&rb, &ref2);
	zassert_equal(write_sz2, write_sz, "Subsequent put_ptr calls returned different sizes");
	zassert_equal(ref, ref2, "Subsequent put_ptr calls returned different pointers");

	memcpy(ref, input, sizeof(input));
	ring_buf_commit(&rb, sizeof(input));

	/* clearing start of buffer to test wrap-around */
	zassert_equal(sizeof(input) - 3, ring_buf_get(&rb, tmp, sizeof(input) - 3),
			"Unexpected read size");
	zassert_true(memcmp(tmp, input, sizeof(input) - 3) == 0, "Data corrupted after get");

	write_sz = ring_buf_put_ptr(&rb, &ref);
	zassert_equal(write_sz, sizeof(buf) - sizeof(input),
			"Unexpected write size, should be able to write up to buffer end");
	zassert_equal(ref, &buf[sizeof(buf) - write_sz],
			"Data pointer incorrect after wrap-around put_ptr");

	write_sz2 = ring_buf_put_ptr(&rb, &ref2);
	zassert_equal(write_sz2, write_sz, "Subsequent put_ptr calls returned different sizes");
	zassert_equal(ref, ref2, "Subsequent put_ptr calls returned different pointers");

	memcpy(ref, &input[0], write_sz);
	ring_buf_commit(&rb, write_sz);

	write_sz = ring_buf_put_ptr(&rb, &ref);
	zassert_equal(write_sz, sizeof(input) - 4,
			"Unexpected write size, should be able to write until data start");
	zassert_equal(ref, buf, "Data pointer incorrect after full buffer wrap-around put_ptr");
	write_sz2 = ring_buf_put_ptr(&rb, &ref2);
	zassert_equal(write_sz2, write_sz, "Subsequent put_ptr calls returned different sizes");
	zassert_equal(ref, ref2, "Subsequent put_ptr calls returned different pointers");
	memcpy(ref, &input[0], write_sz);
	ring_buf_commit(&rb, write_sz);
	zassert_equal(ring_buf_size_get(&rb), sizeof(buf) - 1, "buffer should be full now");
	zassert_equal(ring_buf_space_get(&rb), 0, "no space should be left in buffer");
}


ZTEST(ringbuffer_api, test_ringbuffer_get_ptr)
{
	uint8_t *ref, *ref2;
	size_t read_sz;
	uint8_t buf[16 + 1];
	uint8_t input[13];
	struct ring_buf rb;

	for (uint8_t i = 0; i < sizeof(input); i++) {
		input[i] = i;
	}

	ring_buf_init(&rb, sizeof(buf), buf);

	zassert_equal(0, ring_buf_get_ptr(&rb, &ref), "get_ptr on empty buffer should return 0");
	zassert_equal(sizeof(input), ring_buf_put(&rb, input, sizeof(input)),
			"Unexpected write size");

	read_sz = ring_buf_get_ptr(&rb, &ref);
	zassert_equal(read_sz, sizeof(input), "Unexpected read size");
	zassert_true(memcmp(ref, input, read_sz) == 0, "Data corrupted after get_ptr");
	zassert_equal(ring_buf_size_get(&rb), read_sz, "Size calculation incorrect after get_ptr");
	zassert_equal(ring_buf_get_ptr(&rb, &ref2), read_sz,
			"Subsequent get_ptr calls returned different sizes");
	zassert_equal(ref, ref2, "Subsequent get_ptr calls returned different pointers");
	ring_buf_consume(&rb, read_sz);
	zassert_equal(0, ring_buf_get_ptr(&rb, &ref),
			"get_ptr on empty buffer should return 0 after consume");

	zassert_equal(sizeof(input), ring_buf_put(&rb, input, sizeof(input)),
			"Unexpected write size");

	read_sz = ring_buf_get_ptr(&rb, &ref);
	zassert_equal(sizeof(buf) - sizeof(input), read_sz,
			"get_ptr should return data up to buffer end on wrap-around");
	zassert_equal(ring_buf_get_ptr(&rb, &ref2), read_sz,
			"Subsequent get_ptr calls returned different sizes");
	zassert_equal(ref, ref2, "Subsequent get_ptr calls returned different pointers");
	zassert_true(memcmp(ref, input, read_sz) == 0,
			"Data corrupted after get_ptr on wrap-around");
	ring_buf_consume(&rb, read_sz);

	read_sz = ring_buf_get_ptr(&rb, &ref);
	zassert_equal(ref, buf, "Data pointer incorrect after wrap-around get_ptr");
	zassert_equal(read_sz, sizeof(input) - (sizeof(buf) - sizeof(input)),
			"get_ptr should return remaining data after wrap-around");
	zassert_true(memcmp(ref, &input[sizeof(buf) - sizeof(input)], read_sz) == 0,
			"Data corrupted after get_ptr on wrap-around");
	ring_buf_consume(&rb, read_sz);
	zassert_equal(0, ring_buf_get_ptr(&rb, &ref),
			"get_ptr on empty buffer should return 0 after consuming all data");

	ring_buf_reset(&rb);
	zassert_equal(0, ring_buf_get_ptr(&rb, &ref),
			"get_ptr on empty buffer should return 0 after reset");
	ring_buf_put(&rb, input, sizeof(input));
	zassert_equal(sizeof(input), ring_buf_get(&rb, buf, sizeof(input)), "Unexpected read size");
}


static void verify_init_state(struct ring_buf *rb)
{
	uint8_t *ref;

	zassert_equal(0, ring_buf_size_get(rb), "size should be 0 after init");
	zassert_equal(0, ring_buf_capacity_get(rb), "size should be 0 after init");
	zassert_true(ring_buf_is_empty(rb), "ring buffer should be empty after init");
	zassert_true(ring_buf_is_full(rb), "ring buffer should be full after init");
	zassert_equal(0, ring_buf_get_ptr(rb, &ref), "get_ptr on invalid rb size, should return 0");
	zassert_equal(0, ring_buf_put_ptr(rb, &ref), "put_ptr on invalid rb size, should return 0");
}

static struct ring_buf rbempty = RING_BUF_INIT(NULL, 0);

ZTEST(ringbuffer_api, invalid_size_init)
{
	uint8_t buf[1];
	struct ring_buf rb = RING_BUF_INIT(buf, 0);

	verify_init_state(&rbempty);
	verify_init_state(&rb);
	rb = RING_BUF_INIT(buf, 1);
	verify_init_state(&rb);
}

#include <zephyr/sys/cbprintf.h>
struct tracing_ctx {
	int status;
	uint32_t length;
	struct ring_buf *rb;
};

static int str_put(int c, void *ctx)
{
	struct tracing_ctx *str_ctx = (struct tracing_ctx *)ctx;

	if (str_ctx->status == 0) {
		uint8_t *buf;
		uint32_t claimed_size;

		claimed_size = ring_buf_put_ptr(str_ctx->rb, &buf);
		if (claimed_size) {
			*buf = (uint8_t)c;
			str_ctx->length++;
			ring_buf_commit(str_ctx->rb, 1);
		} else {
			str_ctx->status = -1;
		}
	}

	return 0;
}

static int do_cbvprintf(cbprintf_cb cb, void *ctx, const char *fmt, ...)
{
	va_list ap;
	int rc;

	va_start(ap, fmt);
	rc = cbvprintf(cb, ctx, fmt, ap);
	va_end(ap);

	return rc;
}

ZTEST(ringbuffer_api, claim_finish_cancel)
{
	uint8_t ref[20] = {0};
	uint8_t buf[sizeof(ref)] = {0};
	char str[] = "Hello world!%d";
	const char *expected_str = "Hello world!12";
	struct ring_buf rb = RING_BUF_INIT(buf, sizeof(buf));

	struct ring_buf snapshot = ring_buf_snapshot(&rb);
	struct tracing_ctx str_ctx = {
		.status = 0,
		.length = 0,
		.rb = &snapshot,
	};

	zassert_equal(0, ring_buf_size_get(&rb), "size should be 0 after init");

	(void)do_cbvprintf(str_put, (void *)&str_ctx, str, 12);

	zassert_equal(0, ring_buf_size_get(&rb), "size should not be modified from snapshot");
	zassert_true(str_ctx.length > 0, "some data should have been written to snapshot");
	zassert_true(memcmp(ref, buf, sizeof(ref)) != 0, "buffer should have been modified from snapshot");

	ring_buf_commit(&rb, str_ctx.length);
	zassert_true(str_ctx.length == ring_buf_get(&rb, ref, str_ctx.length), "should be able to read committed data");
	zassert_equal(0, memcmp(expected_str, ref, str_ctx.length), "committed data should match expected");
}

/*test case main entry*/
ZTEST_SUITE(ringbuffer_api, NULL, NULL, NULL, NULL, NULL);
