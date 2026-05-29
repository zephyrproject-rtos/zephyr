/*
 * Copyright (c) 2026 Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/sys/ring_buffer.h>

static uint8_t edge_dummy[1];
static inline void edge_fake_max_init(struct ring_buf *rb)
{
	ring_buf_init(rb, sizeof(edge_dummy), edge_dummy);
	rb->size = RING_BUFFER_MAX_SIZE;
}

ZTEST(ringbuffer_index_math, test_max_size_constants)
{
	struct ring_buf rb;

	zassert_equal(sizeof(rb.size), sizeof(ring_buf_idx_t));
	zassert_equal(sizeof(rb.read_idx), sizeof(ring_buf_idx_t));
	zassert_equal(sizeof(rb.write_idx), sizeof(ring_buf_idx_t));
#ifdef CONFIG_RING_BUFFER_LARGE
	zassert_equal(RING_BUFFER_MAX_SIZE, UINT32_MAX - 1);
#else
	zassert_equal(RING_BUFFER_MAX_SIZE, UINT16_MAX - 1);
#endif
}

ZTEST(ringbuffer_index_math, test_init_at_max_size)
{
	struct ring_buf rb;

	edge_fake_max_init(&rb);
	zassert_equal(ring_buf_capacity_get(&rb), RING_BUFFER_MAX_SIZE - 1);
	zassert_equal(ring_buf_space_get(&rb), RING_BUFFER_MAX_SIZE - 1);
	zassert_equal(ring_buf_size_get(&rb), 0);
	zassert_true(ring_buf_is_empty(&rb));
	zassert_false(ring_buf_is_full(&rb));
}

ZTEST(ringbuffer_index_math, test_fill_completely_max_size)
{
	struct ring_buf rb;
	uint32_t written = 0;
	uint32_t read = 0;
	uint32_t avail;
	uint8_t *p;

	edge_fake_max_init(&rb);

	while ((avail = ring_buf_put_ptr(&rb, &p))) {
		ring_buf_commit(&rb, avail);
		written += avail;
	}
	zassert_equal(written, RING_BUFFER_MAX_SIZE - 1);
	zassert_true(ring_buf_is_full(&rb));
	zassert_equal(ring_buf_size_get(&rb), RING_BUFFER_MAX_SIZE - 1);
	zassert_equal(ring_buf_space_get(&rb), 0);
	zassert_equal(ring_buf_put_ptr(&rb, &p), 0);

	while ((avail = ring_buf_get_ptr(&rb, &p))) {
		ring_buf_consume(&rb, avail);
		read += avail;
	}
	zassert_equal(read, RING_BUFFER_MAX_SIZE - 1);
	zassert_true(ring_buf_is_empty(&rb));
}

ZTEST(ringbuffer_index_math, test_index_at_storage_top)
{
	struct ring_buf rb;

	edge_fake_max_init(&rb);

	rb.read_idx = (ring_buf_idx_t)(rb.size - 2);
	rb.write_idx = (ring_buf_idx_t)(rb.size - 2);
	zassert_true(ring_buf_is_empty(&rb));
	zassert_equal(ring_buf_size_get(&rb), 0);

	ring_buf_commit(&rb, 1);
	zassert_equal(rb.write_idx, rb.size - 1);
	zassert_false(ring_buf_is_full(&rb));
	zassert_equal(ring_buf_size_get(&rb), 1);

	ring_buf_commit(&rb, 1);
	zassert_equal(rb.write_idx, 0);
	zassert_equal(ring_buf_size_get(&rb), 2);

	ring_buf_consume(&rb, 2);
	zassert_true(ring_buf_is_empty(&rb));
	zassert_equal(rb.read_idx, rb.write_idx);
}

ZTEST(ringbuffer_index_math, test_put_ptr_sacrificed_slot_at_top)
{
	struct ring_buf rb;
	uint32_t avail;
	uint8_t *p;

	edge_fake_max_init(&rb);

	avail = ring_buf_put_ptr(&rb, &p);
	zassert_equal(avail, RING_BUFFER_MAX_SIZE - 1);
	zassert_equal(p, edge_dummy);

	ring_buf_commit(&rb, avail);
	zassert_true(ring_buf_is_full(&rb));
	zassert_equal(rb.write_idx, rb.size - 1);
	zassert_equal(rb.read_idx, 0);

	avail = ring_buf_put_ptr(&rb, &p);
	zassert_equal(avail, 0);
}

ZTEST(ringbuffer_index_math, test_size_get_wrap_branch_at_max)
{
	struct ring_buf rb;

	edge_fake_max_init(&rb);

	rb.read_idx = rb.size - 2;
	rb.write_idx = 3;

	zassert_equal(ring_buf_size_get(&rb), 5);
	zassert_equal(ring_buf_space_get(&rb), ring_buf_capacity_get(&rb) - 5);
	zassert_false(ring_buf_is_empty(&rb));
	zassert_false(ring_buf_is_full(&rb));
}

ZTEST(ringbuffer_index_math, test_put_size_uint32_max)
{
	uint8_t buf[16];
	uint8_t src[16];
	struct ring_buf rb;
	uint32_t n;

	for(size_t i = 0; i < sizeof(src); i++) {
		src[i] = (uint8_t)i;
	}
	ring_buf_init(&rb, sizeof(buf), buf);

	n = ring_buf_put(&rb, src, UINT32_MAX);
	zassert_equal(n, ring_buf_capacity_get(&rb));
	zassert_true(ring_buf_is_full(&rb));
	zassert_equal(ring_buf_size_get(&rb), ring_buf_capacity_get(&rb));
}

ZTEST(ringbuffer_index_math, test_get_size_uint32_max)
{
	uint8_t buf[16];
	uint8_t src[15];
	uint8_t dst[15] = {0};
	struct ring_buf rb;
	uint32_t n;

	for (size_t i = 0; i < sizeof(src); i++) {
		src[i] = i;
	}
	ring_buf_init(&rb, sizeof(buf), buf);
	zassert_equal(ring_buf_put(&rb, src, sizeof(src)), sizeof(src));

	n = ring_buf_get(&rb, dst, UINT32_MAX);
	zassert_equal(n, sizeof(dst));
	for (size_t i = 0; i < sizeof(dst); i++) {
		zassert_equal(dst[i], i);
	}
	zassert_true(ring_buf_is_empty(&rb));
}

ZTEST(ringbuffer_index_math, test_peek_size_uint32_max)
{
	uint8_t buf[16];
	uint8_t src[15];
	uint8_t dst[15] = {0};
	struct ring_buf rb;
	uint32_t n;

	for (size_t i = 0; i < sizeof(src); i++) {
		src[i] = i;
	}
	ring_buf_init(&rb, sizeof(buf), buf);
	zassert_equal(ring_buf_put(&rb, src, sizeof(src)), sizeof(src));

	n = ring_buf_peek(&rb, dst, UINT32_MAX);
	zassert_equal(n, sizeof(src));
	for (size_t i = 0; i < sizeof(dst); i++) {
		zassert_equal(dst[i], i);
	}
	zassert_equal(ring_buf_size_get(&rb), sizeof(src));
	zassert_false(ring_buf_is_empty(&rb));
}

ZTEST(ringbuffer_index_math, test_commit_consume_full_storage_walk)
{
	struct ring_buf rb;
	const uint32_t window = 256;

	edge_fake_max_init(&rb);

	//Force the index math to wrap around the underlying index type and verify size/capacity tracking
	rb.read_idx = (ring_buf_idx_t)(RING_BUFFER_MAX_SIZE - (window / 2));
	rb.write_idx = rb.read_idx;

	zassert_true(ring_buf_is_empty(&rb));
	zassert_true(!ring_buf_is_full(&rb));
	for (uint32_t i = 0; i < window; i++) {
		ring_buf_commit(&rb, 1);
		zassert_true(ring_buf_size_get(&rb) == 1, "size should be 1 after commit");
		ring_buf_consume(&rb, 1);
		zassert_true(ring_buf_size_get(&rb) == 0, "size should be 0 after consume");
		zassert_equal(rb.read_idx, rb.write_idx);
	}
	zassert_true(ring_buf_is_empty(&rb));
}

ZTEST_SUITE(ringbuffer_index_math, NULL, NULL, NULL, NULL, NULL);
