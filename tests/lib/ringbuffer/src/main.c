/*
 * Copyright (c) 2026 Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/sys/ring_buffer.h>

#define BUF_SIZE 16
#define DATA_SIZE 12
#define HALF_DATA_SIZE (DATA_SIZE / 2)

ZTEST(ringbuffer_api, test_ringbuffer_copy_api)
{
	uint8_t ring_storage[BUF_SIZE];
	uint8_t input_data[DATA_SIZE];
	uint8_t output_data[BUF_SIZE] = {0};
	struct ring_buf rb;

	for (uint8_t i = 0; i < DATA_SIZE; i++) {
		input_data[i] = i;
	}

	ring_buf_init(&rb, sizeof(ring_storage), ring_storage);

	zassert_equal(ring_buf_get(&rb, output_data, sizeof(output_data)), 0,
		      "Reading from an empty buffer should return 0 bytes");

	zassert_equal(ring_buf_put(&rb, input_data, DATA_SIZE), DATA_SIZE,
		      "Failed to write full input payload");

	zassert_equal(ring_buf_get(&rb, output_data, HALF_DATA_SIZE), HALF_DATA_SIZE,
		      "Failed to read first chunk");
	zassert_mem_equal(output_data, input_data, HALF_DATA_SIZE,
			  "First data chunk corrupted");

	zassert_equal(ring_buf_get(&rb, output_data, HALF_DATA_SIZE), HALF_DATA_SIZE,
		      "Failed to read remaining chunk");
	zassert_mem_equal(output_data, &input_data[HALF_DATA_SIZE], HALF_DATA_SIZE,
			  "Remaining data chunk corrupted");

	zassert_equal(ring_buf_get(&rb, output_data, 1), 0,
		      "Buffer should be empty after reading all contents");

	zassert_equal(ring_buf_put(&rb, input_data, 8), 8, "Failed to write data");
	zassert_equal(ring_buf_get(&rb, output_data, 8), 8, "Failed to flush data");

	zassert_equal(ring_buf_put(&rb, input_data, DATA_SIZE), DATA_SIZE,
		      "Failed to write data during wrap-around state");
	zassert_equal(ring_buf_get(&rb, output_data, DATA_SIZE), DATA_SIZE,
		      "Failed to read data during wrap-around state");
	zassert_mem_equal(output_data, input_data, DATA_SIZE,
			  "Data corrupted during wrap-around copy handling");

	ring_buf_reset(&rb);
	zassert_equal(ring_buf_get(&rb, output_data, 1), 0,
		      "Buffer should return 0 bytes immediately after reset");

	zassert_equal(ring_buf_put(&rb, output_data, BUF_SIZE), BUF_SIZE,
		      "Buffer capacity was not fully recovered after reset");
}

ZTEST(ringbuffer_api, test_ringbuffer_zero_copy)
{
	uint8_t *ref, *ref2;
	size_t read_sz;
	size_t write_sz;
	uint8_t buf[16];
	uint8_t input[12];
	struct ring_buf rb;

	for (uint8_t i = 0; i < sizeof(input); i++) {
		input[i] = i;
	}

	ring_buf_init(&rb, sizeof(buf), buf);

	ring_buf_put(&rb, input, sizeof(input));

	ref = NULL;
	read_sz = ring_buf_get_ptr(&rb, &ref, 0);
	zassert_equal(read_sz, sizeof(input), "Unexpected read size");
	zassert_true(memcmp(ref, input, read_sz) == 0, "Data corrupted after get_ptr");
	zassert_equal(ring_buf_size_get(&rb), read_sz, "Size calculation incorrect after get_ptr");
	zassert_equal(ring_buf_capacity_get(&rb), sizeof(buf), "Capacity changed after get_ptr");
	zassert_equal(ring_buf_space_get(&rb), sizeof(buf) - read_sz,
			"Space calculation incorrect after get_ptr");

	zassert_equal(ring_buf_get_ptr(&rb, &ref2, 0), read_sz,
			"Subsequent get_ptr calls returned different sizes");
	zassert_equal(ref, ref2, "Subsequent get_ptr calls returned different pointers");

	ring_buf_consume(&rb, read_sz);
	zassert_equal(ring_buf_size_get(&rb), 0, "Size not zero after consuming all data");
	zassert_equal(ring_buf_capacity_get(&rb), sizeof(buf), "Capacity changed after consume");
	zassert_equal(ring_buf_space_get(&rb), sizeof(buf),
			"Space not fully restored after consuming all data");
	zassert_equal(ring_buf_get_ptr(&rb, &ref, 0), 0,
			"Data still available after consuming all data");

	write_sz = ring_buf_put_ptr(&rb, &ref, 0);
	zassert_equal(write_sz, sizeof(buf) - sizeof(input),
			"Unexpected write size");
	zassert_equal(ring_buf_size_get(&rb), 0, "Change in data without commit");
	zassert_equal(ring_buf_capacity_get(&rb), sizeof(buf), "Capacity changed after put_ptr");
	zassert_equal(ring_buf_space_get(&rb), sizeof(buf), "Space changed after put_ptr");

	zassert_equal(ring_buf_put_ptr(&rb, &ref2, 0), write_sz,
			"Subsequent put_ptr calls returned different sizes");
	zassert_equal(ref, ref2, "Subsequent put_ptr calls returned different pointers");

	memcpy(ref, &input[0], write_sz);
	ring_buf_commit(&rb, write_sz);
	zassert_equal(ring_buf_size_get(&rb), write_sz,
			"Size not updated after commit");

	zassert_equal(ring_buf_get_ptr(&rb, &ref, 0), write_sz, "Data size incorrect after commit");
	zassert_true(memcmp(ref, &input[0], write_sz) == 0, "Data corrupted after commit");
	ring_buf_consume(&rb, write_sz);

	write_sz = ring_buf_put_ptr(&rb, &ref, 0);
	zassert_equal(ref, buf, "Data pointer incorrect after buffer empty");
	zassert_equal(write_sz, sizeof(buf),
			"Unexpected write size after buffer empty");
	memcpy(ref, input, sizeof(buf) / 2);
	ring_buf_commit(&rb, sizeof(buf) / 2);

	zassert_equal(ring_buf_size_get(&rb), sizeof(buf) / 2,
			"Size not updated after partial commit");
	zassert_equal(ring_buf_space_get(&rb), sizeof(buf) / 2,
			"Space not updated after partial commit");
	zassert_equal(ring_buf_get_ptr(&rb, &ref, 0), sizeof(buf) / 2,
			"Data size incorrect after partial commit");
	zassert_equal(ref, buf, "Data pointer incorrect after partial commit");
}


ZTEST(ringbuffer_api, test_ringbuffer_put_ptr)
{
	uint8_t *ref, *ref2;
	size_t write_sz, write_sz2;
	uint8_t buf[16];
	uint8_t tmp[16];
	uint8_t input[13];
	struct ring_buf rb;

	for (uint8_t i = 0; i < sizeof(input); i++) {
		input[i] = i;
	}

	ring_buf_init(&rb, sizeof(buf), buf);

	write_sz = ring_buf_put_ptr(&rb, &ref, 0);
	zassert_equal(write_sz, sizeof(buf), "Unexpected write size");
	write_sz2 = ring_buf_put_ptr(&rb, &ref2, 0);
	zassert_equal(write_sz2, write_sz, "Subsequent put_ptr calls returned different sizes");
	zassert_equal(ref, ref2, "Subsequent put_ptr calls returned different pointers");

	memcpy(ref, input, sizeof(input));
	ring_buf_commit(&rb, sizeof(input));

	/* clearing start of buffer to test wrap-around */
	zassert_equal(sizeof(input) - 3, ring_buf_get(&rb, tmp, sizeof(input) - 3),
			"Unexpected read size");
	zassert_true(memcmp(tmp, input, sizeof(input) - 3) == 0, "Data corrupted after get");

	write_sz = ring_buf_put_ptr(&rb, &ref, 0);
	zassert_equal(write_sz, sizeof(buf) - sizeof(input),
			"Unexpected write size, should be able to write up to buffer end");
	zassert_equal(ref, &buf[sizeof(buf) - write_sz],
			"Data pointer incorrect after wrap-around put_ptr");

	write_sz2 = ring_buf_put_ptr(&rb, &ref2, 0);
	zassert_equal(write_sz2, write_sz, "Subsequent put_ptr calls returned different sizes");
	zassert_equal(ref, ref2, "Subsequent put_ptr calls returned different pointers");

	memcpy(ref, &input[0], write_sz);
	ring_buf_commit(&rb, write_sz);

	write_sz = ring_buf_put_ptr(&rb, &ref, 0);
	zassert_equal(write_sz, sizeof(input) - 3,
			"Unexpected write size, should be able to write until data start");
	zassert_equal(ref, buf, "Data pointer incorrect after full buffer wrap-around put_ptr");
	write_sz2 = ring_buf_put_ptr(&rb, &ref2, 0);
	zassert_equal(write_sz2, write_sz, "Subsequent put_ptr calls returned different sizes");
	zassert_equal(ref, ref2, "Subsequent put_ptr calls returned different pointers");
	memcpy(ref, &input[0], write_sz);
	ring_buf_commit(&rb, write_sz);
	zassert_equal(ring_buf_size_get(&rb), sizeof(buf), "buffer should be full now");
	zassert_equal(ring_buf_space_get(&rb), 0, "no space should be left in buffer");
}


ZTEST(ringbuffer_api, test_ringbuffer_get_ptr)
{
	uint8_t *ref, *ref2;
	size_t read_sz;
	uint8_t buf[16];
	uint8_t input[13];
	struct ring_buf rb;

	for (uint8_t i = 0; i < sizeof(input); i++) {
		input[i] = i;
	}

	ring_buf_init(&rb, sizeof(buf), buf);

	zassert_equal(0, ring_buf_get_ptr(&rb, &ref, 0), "get_ptr on empty buffer should return 0");
	zassert_equal(sizeof(input), ring_buf_put(&rb, input, sizeof(input)),
			"Unexpected write size");

	read_sz = ring_buf_get_ptr(&rb, &ref, 0);
	zassert_equal(read_sz, sizeof(input), "Unexpected read size");
	zassert_true(memcmp(ref, input, read_sz) == 0, "Data corrupted after get_ptr");
	zassert_equal(ring_buf_size_get(&rb), read_sz, "Size calculation incorrect after get_ptr");
	zassert_equal(ring_buf_get_ptr(&rb, &ref2, 0), read_sz,
			"Subsequent get_ptr calls returned different sizes");
	zassert_equal(ref, ref2, "Subsequent get_ptr calls returned different pointers");
	ring_buf_consume(&rb, read_sz);
	zassert_equal(0, ring_buf_get_ptr(&rb, &ref, 0),
			"get_ptr on empty buffer should return 0 after consume");

	zassert_equal(sizeof(input), ring_buf_put(&rb, input, sizeof(input)),
			"Unexpected write size");

	read_sz = ring_buf_get_ptr(&rb, &ref, 0);
	zassert_equal(sizeof(buf) - sizeof(input), read_sz,
			"get_ptr should return data up to buffer end on wrap-around");
	zassert_equal(ring_buf_get_ptr(&rb, &ref2, 0), read_sz,
			"Subsequent get_ptr calls returned different sizes");
	zassert_equal(ref, ref2, "Subsequent get_ptr calls returned different pointers");
	zassert_true(memcmp(ref, input, read_sz) == 0,
			"Data corrupted after get_ptr on wrap-around");
	ring_buf_consume(&rb, read_sz);

	read_sz = ring_buf_get_ptr(&rb, &ref, 0);
	zassert_equal(ref, buf, "Data pointer incorrect after wrap-around get_ptr");
	zassert_equal(read_sz, sizeof(input) - (sizeof(buf) - sizeof(input)),
			"get_ptr should return remaining data after wrap-around");
	zassert_true(memcmp(ref, &input[sizeof(buf) - sizeof(input)], read_sz) == 0,
			"Data corrupted after get_ptr on wrap-around");
	ring_buf_consume(&rb, read_sz);
	zassert_equal(0, ring_buf_get_ptr(&rb, &ref, 0),
			"get_ptr on empty buffer should return 0 after consuming all data");

	ring_buf_reset(&rb);
	zassert_equal(0, ring_buf_get_ptr(&rb, &ref, 0),
			"get_ptr on empty buffer should return 0 after reset");
	ring_buf_put(&rb, input, sizeof(input));
	zassert_equal(sizeof(input), ring_buf_get(&rb, input, sizeof(input)),
		"Unexpected read size");
}

ZTEST(ringbuffer_api, test_ringbuffer_put_ptr_offset)
{
	uint8_t *ref;
	uint32_t avail;
	uint8_t buf[16];
	struct ring_buf rb;

	ring_buf_init(&rb, sizeof(buf), buf);

	avail = ring_buf_put_ptr(&rb, &ref, 0);
	zassert_equal(avail, sizeof(buf), "offset 0 should expose full buffer");
	zassert_equal(ref, buf, "offset 0 should point at buffer start");

	avail = ring_buf_put_ptr(&rb, &ref, 4);
	zassert_equal(ref, &buf[4], "offset should advance the pointer");
	zassert_equal(avail, sizeof(buf) - 4, "offset should shrink available space");
	zassert_equal(ring_buf_size_get(&rb), 0, "offset must not mutate the buffer");
	zassert_equal(ring_buf_space_get(&rb), sizeof(buf), "offset must not consume space");

	avail = ring_buf_put_ptr(&rb, &ref, sizeof(buf));
	zassert_equal(ref, buf, "offset at capacity should wrap to start");
	zassert_equal(avail, 0, "no free space remains past a full reservation");
}

ZTEST(ringbuffer_api, test_ringbuffer_get_ptr_offset)
{
	uint8_t *ref;
	uint32_t avail;
	uint8_t buf[16];
	uint8_t input[16];
	struct ring_buf rb;

	for (uint8_t i = 0; i < sizeof(input); i++) {
		input[i] = i;
	}

	ring_buf_init(&rb, sizeof(buf), buf);
	ring_buf_put(&rb, input, sizeof(input));
	zassert_equal(8, ring_buf_get(&rb, input, 8), "Unexpected read size");
	zassert_equal(8, ring_buf_put(&rb, input, 8), "Unexpected write size");

	avail = ring_buf_get_ptr(&rb, &ref, 0);
	zassert_equal(avail, sizeof(buf) - 8, "offset 0 should reach buffer end");
	zassert_equal(ref, &buf[8], "offset 0 should point at read index");

	avail = ring_buf_get_ptr(&rb, &ref, 4);
	zassert_equal(ref, &buf[12], "offset should advance the read pointer");
	zassert_equal(avail, sizeof(buf) - 12, "offset should shrink available data");

	avail = ring_buf_get_ptr(&rb, &ref, sizeof(buf) - 8);
	zassert_equal(ref, buf, "offset across wrap should point at start");
	zassert_equal(avail, 8, "remaining data after wrap");

	zassert_equal(ring_buf_size_get(&rb), sizeof(buf), "offset must not consume data");
}

static void check_peek(struct ring_buf *rb, const uint8_t *expected, uint32_t len)
{
	uint8_t output[BUF_SIZE + 1];

	zassert_true(len < sizeof(output), "Test setup error: output buffer too small");

	memset(output, 0xff, sizeof(output));
	zassert_equal(ring_buf_peek(rb, output, HALF_DATA_SIZE), HALF_DATA_SIZE,
		      "Partial peek should return the requested size");
	zassert_mem_equal(output, expected, HALF_DATA_SIZE, "Partial peek returned corrupted data");

	memset(output, 0xff, sizeof(output));
	zassert_equal(ring_buf_peek(rb, output, sizeof(output)), len,
		      "Peek larger than available should return only available bytes");
	zassert_mem_equal(output, expected, len, "Peek returned corrupted data");
	zassert_equal(output[len], 0xff, "Peek wrote past the available bytes");

	memset(output, 0xff, sizeof(output));
	zassert_equal(ring_buf_peek(rb, output, len), len,
		      "Repeated peek returned a different size");
	zassert_mem_equal(output, expected, len, "Repeated peek returned different data");

	zassert_equal(ring_buf_size_get(rb), len, "Peek must not consume data");

	zassert_equal(ring_buf_get(rb, output, len), len,
		      "Peeked data should still be retrievable");
	zassert_mem_equal(output, expected, len, "Get after peek returned corrupted data");
	zassert_equal(ring_buf_size_get(rb), 0, "Buffer should be empty after get");
}

ZTEST(ringbuffer_api, test_ringbuffer_peek)
{
	uint8_t ring_storage[BUF_SIZE];
	uint8_t input_data[DATA_SIZE];
	struct ring_buf rb;

	for (uint8_t i = 0; i < DATA_SIZE; i++) {
		input_data[i] = i;
	}

	ring_buf_init(&rb, sizeof(ring_storage), ring_storage);
	zassert_equal(ring_buf_put(&rb, input_data, DATA_SIZE), DATA_SIZE,
		      "Failed to write input payload");

	check_peek(&rb, input_data, DATA_SIZE);
}

ZTEST(ringbuffer_api, test_ringbuffer_peek_wrap)
{
	uint8_t ring_storage[BUF_SIZE];
	uint8_t input_data[DATA_SIZE];
	uint8_t scratch[DATA_SIZE];
	struct ring_buf rb;

	BUILD_ASSERT(DATA_SIZE < BUF_SIZE && 2 * DATA_SIZE > BUF_SIZE,
		     "Setup relies on the payload straddling the physical buffer end");

	for (uint8_t i = 0; i < DATA_SIZE; i++) {
		input_data[i] = i;
	}

	ring_buf_init(&rb, sizeof(ring_storage), ring_storage);

	/* prime ring_buffer to produce wrap case for peek */
	zassert_equal(ring_buf_put(&rb, input_data, DATA_SIZE), DATA_SIZE,
		      "Failed to prime buffer");
	zassert_equal(ring_buf_get(&rb, scratch, DATA_SIZE), DATA_SIZE, "Failed to drain buffer");
	zassert_equal(ring_buf_put(&rb, input_data, DATA_SIZE), DATA_SIZE,
		      "Failed to write input payload");

	check_peek(&rb, input_data, DATA_SIZE);
}

ZTEST_SUITE(ringbuffer_api, NULL, NULL, NULL, NULL, NULL);
