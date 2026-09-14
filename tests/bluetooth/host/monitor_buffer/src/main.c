/*
 * Copyright (c) 2026 Silicon Laboratories Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>
#include <string.h>

#include <zephyr/fff.h>
#include <zephyr/sys/ring_buffer.h>
#include <zephyr/ztest.h>

#include "monitor_buffer.h"

DEFINE_FFF_GLOBALS;

ZTEST_SUITE(bt_monitor_buffer, NULL, NULL, NULL, NULL, NULL);

static void ring_put(struct ring_buf *buf, const uint8_t *data, size_t len)
{
	zassert_equal(ring_buf_put(buf, data, len), len);
}

static void ring_get(struct ring_buf *buf, uint8_t *data, size_t len)
{
	zassert_equal(ring_buf_get(buf, data, len), len);
}

ZTEST(bt_monitor_buffer, test_put_fragments)
{
	uint8_t storage[16];
	struct ring_buf buf;
	static const uint8_t header[] = { 0x01, 0x02, 0x03 };
	static const uint8_t payload[] = { 0x04, 0x05, 0x06, 0x07 };
	static const uint8_t expected[] = { 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07 };
	const struct bt_monitor_data frags[] = {
		{ header, sizeof(header) },
		{ payload, sizeof(payload) },
	};
	uint8_t output[sizeof(expected)];

	ring_buf_init(&buf, sizeof(storage), storage);

	zassert_true(bt_monitor_ring_buf_put(&buf, frags, ARRAY_SIZE(frags)));
	zassert_equal(ring_buf_size_get(&buf), sizeof(expected));
	ring_get(&buf, output, sizeof(output));
	zassert_mem_equal(output, expected, sizeof(expected));
}

ZTEST(bt_monitor_buffer, test_put_fragments_across_wrap)
{
	uint8_t storage[16];
	struct ring_buf buf;
	static const uint8_t initial[] = { 0x00, 0x01, 0x02, 0x03, 0x04,
					   0x05, 0x06, 0x07, 0x08, 0x09 };
	static const uint8_t header[] = { 0x10, 0x11, 0x12, 0x13 };
	static const uint8_t payload[] = { 0x20, 0x21, 0x22, 0x23, 0x24, 0x25 };
	static const uint8_t expected[] = { 0x08, 0x09, 0x10, 0x11, 0x12, 0x13,
					    0x20, 0x21, 0x22, 0x23, 0x24, 0x25 };
	const struct bt_monitor_data frags[] = {
		{ header, sizeof(header) },
		{ payload, sizeof(payload) },
	};
	uint8_t output[sizeof(expected)];
	uint8_t discarded[8];

	ring_buf_init(&buf, sizeof(storage), storage);
	ring_put(&buf, initial, sizeof(initial));
	ring_get(&buf, discarded, sizeof(discarded));

	zassert_true(bt_monitor_ring_buf_put(&buf, frags, ARRAY_SIZE(frags)));
	ring_get(&buf, output, sizeof(output));
	zassert_mem_equal(output, expected, sizeof(expected));
}

ZTEST(bt_monitor_buffer, test_insufficient_space_does_not_publish_partial_record)
{
	uint8_t storage[8];
	struct ring_buf buf;
	static const uint8_t initial[] = { 0xaa, 0xbb, 0xcc };
	static const uint8_t header[] = { 0x01, 0x02, 0x03 };
	static const uint8_t payload[] = { 0x04, 0x05, 0x06 };
	const struct bt_monitor_data frags[] = {
		{ header, sizeof(header) },
		{ payload, sizeof(payload) },
	};
	uint8_t output[sizeof(initial)];

	ring_buf_init(&buf, sizeof(storage), storage);
	ring_put(&buf, initial, sizeof(initial));

	zassert_false(bt_monitor_ring_buf_put(&buf, frags, ARRAY_SIZE(frags)));
	zassert_equal(ring_buf_size_get(&buf), sizeof(initial));
	ring_get(&buf, output, sizeof(output));
	zassert_mem_equal(output, initial, sizeof(initial));
}

ZTEST(bt_monitor_buffer, test_zero_length_fragment_with_null_data)
{
	uint8_t storage[8];
	struct ring_buf buf;
	static const uint8_t header[] = { 0x01, 0x02 };
	const struct bt_monitor_data frags[] = {
		{ header, sizeof(header) },
		{ NULL, 0 },
	};
	uint8_t output[sizeof(header)];

	ring_buf_init(&buf, sizeof(storage), storage);

	zassert_true(bt_monitor_ring_buf_put(&buf, frags, ARRAY_SIZE(frags)));
	zassert_equal(ring_buf_size_get(&buf), sizeof(header));
	ring_get(&buf, output, sizeof(output));
	zassert_mem_equal(output, header, sizeof(header));
}

ZTEST(bt_monitor_buffer, test_size_max_fragment_is_rejected)
{
	uint8_t storage[8];
	struct ring_buf buf;
	static const uint8_t header[] = { 0x01, 0x02 };
	/* A SIZE_MAX length would wrap the fragment size sum if it were
	 * computed before being checked; the length must be rejected without
	 * the data pointer ever being dereferenced.
	 */
	const struct bt_monitor_data frags[] = {
		{ header, sizeof(header) },
		{ header, SIZE_MAX },
	};

	ring_buf_init(&buf, sizeof(storage), storage);

	zassert_false(bt_monitor_ring_buf_put(&buf, frags, ARRAY_SIZE(frags)));
	zassert_true(ring_buf_is_empty(&buf));
}

ZTEST(bt_monitor_buffer, test_fragment_larger_than_remaining_space)
{
	uint8_t storage[8];
	struct ring_buf buf;
	static const uint8_t header[] = { 0x01, 0x02, 0x03, 0x04, 0x05 };
	static const uint8_t payload[] = { 0x06, 0x07, 0x08, 0x09 };
	const struct bt_monitor_data frags[] = {
		{ header, sizeof(header) },
		{ payload, sizeof(payload) },
	};

	ring_buf_init(&buf, sizeof(storage), storage);

	/* The first fragment fits on its own but the second exceeds what is
	 * left after it; the whole record must be rejected.
	 */
	zassert_false(bt_monitor_ring_buf_put(&buf, frags, ARRAY_SIZE(frags)));
	zassert_true(ring_buf_is_empty(&buf));
}

ZTEST(bt_monitor_buffer, test_record_exactly_filling_buffer)
{
	uint8_t storage[8];
	struct ring_buf buf;
	static const uint8_t header[] = { 0x01, 0x02, 0x03 };
	static const uint8_t payload[] = { 0x04, 0x05, 0x06, 0x07, 0x08 };
	static const uint8_t expected[] = { 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08 };
	const struct bt_monitor_data frags[] = {
		{ header, sizeof(header) },
		{ payload, sizeof(payload) },
	};
	uint8_t output[sizeof(expected)];

	ring_buf_init(&buf, sizeof(storage), storage);

	zassert_true(bt_monitor_ring_buf_put(&buf, frags, ARRAY_SIZE(frags)));
	zassert_equal(ring_buf_size_get(&buf), sizeof(expected));
	zassert_equal(ring_buf_space_get(&buf), 0);
	ring_get(&buf, output, sizeof(output));
	zassert_mem_equal(output, expected, sizeof(expected));
}
