/*
 * Copyright (c) 2025 Måns Ansgariusson <mansgariusson@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/random/random.h>
#include <zephyr/sys/ring_buffer.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(ring_buffer_test, LOG_LEVEL_DBG);

ZTEST_SUITE(ringbuffer_api, NULL, NULL, NULL, NULL, NULL);

ZTEST(ringbuffer_api, test_init)
{
	uint8_t buffer[12];
	struct ring_buffer rb;

	ring_buffer_init(&rb, buffer, sizeof(buffer));

	zassert_true(rb.buffer == buffer, "Incorrect buffer pointer");
	zassert_true(ring_buffer_size(&rb) == 0, "No bytes should be available");
	zassert_true(ring_buffer_capacity(&rb) == sizeof(buffer), "capacity != input size");
}

ZTEST(ringbuffer_api, test_io)
{
	struct ring_buffer rb;
	uint8_t buffer[12];
	uint8_t input[12];
	uint8_t res[12];

	sys_rand_get(input, sizeof(input));
	ring_buffer_init(&rb, buffer, sizeof(buffer));

	zassert_true(sizeof(input) == ring_buffer_write(&rb, input, sizeof(input)),
			"Failed to write input to ring_buffer");
	zassert_true(ring_buffer_size(&rb) == sizeof(input),
			"ring_buffer should have written bytes available for reading");
	zassert_true(sizeof(res) == ring_buffer_read(&rb, res, sizeof(res)),
			"failed to read written bytes");
	zassert_true(!memcmp(input, res, sizeof(input)), "read != written");
	zassert_true(ring_buffer_size(&rb) == 0, "ring_buffer should be empty");
}

ZTEST(ringbuffer_api, test_dma_io)
{
	size_t read = 0;
	size_t written = 0;
	struct ring_buffer rb;
	uint8_t buffer[12];
	uint8_t input[12];
	uint8_t res[12];
	uint8_t *ref;

	sys_rand_get(input, sizeof(input));
	ring_buffer_init(&rb, buffer, sizeof(buffer));

	while (written < sizeof(input)) {
		size_t write_sz = MIN(ring_buffer_write_ptr(&rb, &ref), sizeof(input) - written);

		zassert_true(write_sz > 0, "there should be space in the buffer");
		memcpy(ref, &input[written], write_sz);
		ring_buffer_commit(&rb, write_sz);
		written += write_sz;
	}

	zassert_true(sizeof(input) == ring_buffer_size(&rb),
			"there should be written bytes available to read");

	while (read < sizeof(res)) {
		size_t read_sz = MIN(ring_buffer_read_ptr(&rb, &ref), sizeof(input) - read);

		zassert_true(read_sz > 0, "there should be data in the buffer");
		memcpy(&res[read], ref, read_sz);
		ring_buffer_consume(&rb, read_sz);
		read += read_sz;
	}

	zassert_true(!memcmp(input, res, sizeof(input)), "read != written");
	zassert_true(ring_buffer_size(&rb) == 0, "no bytes should be available to read");
}

#ifndef CONFIG_RING_BUFFER_CONDITIONAL
#define MONOTONIC_MAX_VALUE ((ring_buffer_index_t) -1)
ZTEST(ringbuffer_api, test_index_overflow)
{
	struct ring_buffer rb;
	uint8_t buffer[12];
	uint8_t input[12];
	uint8_t res[12];

	sys_rand_get(input, sizeof(input));
	ring_buffer_init(&rb, buffer, sizeof(buffer));

	/* Moving the monotonic counters to the end of their range. */
	rb.write_ptr = MONOTONIC_MAX_VALUE - 3;
	rb.read_ptr = MONOTONIC_MAX_VALUE - 3;

	zassert_true(sizeof(input) == ring_buffer_write(&rb, input, sizeof(input)),
			"Failed to write input to ring_buffer");
	zassert_true(ring_buffer_size(&rb) == sizeof(input),
			"ring_buffer should have written bytes available for reading");
	zassert_true(sizeof(res) == ring_buffer_read(&rb, res, sizeof(res)),
			"failed to read written bytes");
	zassert_true(!memcmp(input, res, sizeof(input)), "read != written");
	zassert_true(ring_buffer_size(&rb) == 0, "ring_buffer should be empty");
	zassert_true(ring_buffer_space(&rb) == ring_buffer_capacity(&rb),
	      "buffer should be empty, full space");
}
#endif /* CONFIG_RING_BUFFER_CONDITIONAL */

ZTEST(ringbuffer_api, test_stress)
{
	struct ring_buffer rb;
	uint8_t buffer[12];
	uint8_t input[128];
	uint8_t res[128];
	size_t read = 0;
	size_t written = 0;
	size_t to_write;
	size_t to_read;

	sys_rand_get(input, sizeof(input));
	ring_buffer_init(&rb, buffer, sizeof(buffer));

	while (read < sizeof(input)) {
		to_write = MIN(sys_rand32_get() % (ring_buffer_space(&rb) + 1),
				sizeof(input) - written);
		zassert_true(ring_buffer_write(&rb, &input[written], to_write) == to_write,
				"Failed to write");
		written += to_write;

		zassert_true(ring_buffer_size(&rb) == to_write,
			"buffer should contain written data");
		zassert_true(ring_buffer_space(&rb) == ring_buffer_capacity(&rb) - to_write,
			"buffer should contain written data");
		while (read < written) {
			to_read = MIN(sys_rand32_get() % (ring_buffer_size(&rb) + 1),
					written - read);
			zassert_true(ring_buffer_read(&rb, &res[read], to_read) == to_read,
					"Failed to read");
			read += to_read;
		}
		zassert_true(ring_buffer_size(&rb) == 0, "buffer should be empty");
		zassert_true(ring_buffer_space(&rb) == ring_buffer_capacity(&rb),
			"buffer should be empty, full space");
	}
	zassert_true(!memcmp(input, res, read), "read != written");
}

/* Performance measurement results should only be considered if ASSERTS are disabled */
ZTEST(ringbuffer_api, test_ringbuffer_performance)
{
	struct ring_buffer rb;
	uint8_t buffer[12];
	uint8_t indata[16];
	uint8_t outdata[16];
	uint8_t *ptr;
	uint32_t timestamp;
	int loop = 1000;

	ring_buffer_init(&rb, buffer, sizeof(buffer));

	timestamp = k_cycle_get_32();
	for (int i = 0; i < loop; i++) {
		ring_buffer_write(&rb, indata, 1);
		ring_buffer_read(&rb, outdata, 1);
	}
	timestamp =  k_cycle_get_32() - timestamp;
	LOG_INF("1 byte write+read, avg cycles: %d", timestamp/loop);

	ring_buffer_reset(&rb);
	timestamp = k_cycle_get_32();
	for (int i = 0; i < loop; i++) {
		ring_buffer_write(&rb, indata, 4);
		ring_buffer_read(&rb, outdata, 4);
	}
	timestamp =  k_cycle_get_32() - timestamp;
	LOG_INF("4 byte write+read, avg cycles: %d", timestamp/loop);

	ring_buffer_reset(&rb);
	timestamp = k_cycle_get_32();
	for (int i = 0; i < loop; i++) {
		ring_buffer_write_ptr(&rb, &ptr);
		ring_buffer_commit(&rb, 1);
		ring_buffer_read(&rb, outdata, 1);
	}
	timestamp =  k_cycle_get_32() - timestamp;
	LOG_INF("1 byte write_ptr+commit+read, avg cycles: %d", timestamp/loop);

	ring_buffer_reset(&rb);
	timestamp = k_cycle_get_32();
	for (int i = 0; i < loop; i++) {
		ring_buffer_write_ptr(&rb, &ptr);
		ring_buffer_commit(&rb, 5);
		ring_buffer_read(&rb, outdata, 5);
	}
	timestamp =  k_cycle_get_32() - timestamp;
	LOG_INF("5 byte write_ptr+commit+read, avg cycles: %d", timestamp/loop);

	ring_buffer_reset(&rb);
	timestamp = k_cycle_get_32();
	for (int i = 0; i < loop; i++) {
		ring_buffer_write(&rb, indata, 5);
		ring_buffer_read_ptr(&rb, &ptr);
		ring_buffer_consume(&rb, 5);
	}
	timestamp =  k_cycle_get_32() - timestamp;
	LOG_INF("5 byte write+read_ptr+consume, avg cycles: %d", timestamp/loop);
}
