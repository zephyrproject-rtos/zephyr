/*
 * Copyright (c) 2026 Silicon Laboratories Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SUBSYS_BLUETOOTH_HOST_MONITOR_BUFFER_H_
#define ZEPHYR_SUBSYS_BLUETOOTH_HOST_MONITOR_BUFFER_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <zephyr/sys/barrier.h>
#include <zephyr/sys/ring_buffer.h>
#include <zephyr/sys/util.h>

struct bt_monitor_data {
	const void *data;
	size_t len;
};

/* Header-only so that the unit test in tests/bluetooth/host/monitor_buffer
 * can build this function without pulling in monitor.c and its dependencies.
 * monitor.c is the only in-tree includer, so the generated code is the same
 * as for a file-local static function.
 */
static inline bool bt_monitor_ring_buf_put(struct ring_buf *buf,
					   const struct bt_monitor_data *frags, size_t count)
{
	size_t space = ring_buf_space_get(buf);
	size_t total = 0;

	/* total only grows after passing the check, so total <= space is
	 * invariant: the subtraction cannot underflow, the sum cannot
	 * overflow, and offset/remaining in the copy loop below stay bounded
	 * by the buffer size.
	 */
	for (size_t i = 0; i < count; i++) {
		if (frags[i].len > space - total) {
			return false;
		}

		total += frags[i].len;
	}

	/* Pairs with bt_monitor_ring_buf_consume(): space observed above may
	 * have been freed by a consumer on another CPU, whose data reads must
	 * complete before the writes below reuse it.
	 */
	barrier_dmem_fence_full();

	/* The space check guarantees that the writes below always fit, and
	 * the single commit makes the complete record visible to the
	 * consumer at once.
	 */
	for (size_t i = 0, offset = 0; i < count; i++) {
		const uint8_t *src = frags[i].data;
		size_t remaining = frags[i].len;

		while (remaining > 0) {
			uint8_t *dst;
			size_t len;

			len = MIN(remaining, ring_buf_put_ptr(buf, &dst, offset));
			(void)memcpy(dst, src, len);
			offset += len;
			src += len;
			remaining -= len;
		}
	}

	/* Make the record bytes visible before the commit publishes them
	 * (pairs with bt_monitor_ring_buf_get_ptr()).
	 */
	barrier_dmem_fence_full();
	ring_buf_commit(buf, total);

	return true;
}

/* Consumer-side wrappers adding the memory ordering that the ring buffer's
 * lock-free SPSC contract leaves to its users on SMP systems. The producer
 * side is handled in bt_monitor_ring_buf_put().
 */
static inline uint32_t bt_monitor_ring_buf_get_ptr(struct ring_buf *buf, uint8_t **data)
{
	uint32_t len = ring_buf_get_ptr(buf, data, 0);

	/* Order the index load against the data reads that follow */
	barrier_dmem_fence_full();

	return len;
}

static inline void bt_monitor_ring_buf_consume(struct ring_buf *buf, uint32_t size)
{
	/* Order the data reads against the space becoming reusable */
	barrier_dmem_fence_full();
	ring_buf_consume(buf, size);
}

#endif /* ZEPHYR_SUBSYS_BLUETOOTH_HOST_MONITOR_BUFFER_H_ */
