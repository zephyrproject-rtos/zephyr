/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Process Mission
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stdint.h>

#include <zephyr/drivers/virtio.h>
#include <zephyr/drivers/virtio/virtqueue.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>
#include <zephyr/ztest.h>

#include "virtio_common.h"

#define VQ_SIZE 8U

static struct virtq *test_get_virtqueue(const struct device *dev, uint16_t queue_idx)
{
	ARG_UNUSED(queue_idx);

	return dev->data;
}

static DEVICE_API(virtio, test_virtio_api) = {
	.get_virtqueue = test_get_virtqueue,
};

static uint32_t callback_used_len;
static uint16_t callback_count;

static void complete_cb(void *opaque, uint32_t used_len)
{
	ARG_UNUSED(opaque);

	callback_used_len = used_len;
	callback_count++;
}

static void fill_bufs(struct virtq_buf *bufs, size_t count, uint8_t *storage)
{
	for (size_t i = 0; i < count; i++) {
		bufs[i].addr = &storage[i];
		bufs[i].len = 1;
	}
}

ZTEST(virtqueue, test_chain_allocation_settles_free_count)
{
	static uint8_t storage[3];
	struct virtq_buf bufs[ARRAY_SIZE(storage)];
	struct virtq vq;
	uint16_t head;

	zassert_ok(virtq_create(&vq, VQ_SIZE));
	zassert_equal(vq.free_desc_n, VQ_SIZE);

	fill_bufs(bufs, ARRAY_SIZE(bufs), storage);
	zassert_ok(virtq_add_buffer_chain(&vq, bufs, ARRAY_SIZE(bufs), 0, NULL, NULL, K_NO_WAIT));
	zassert_equal(vq.free_desc_n, VQ_SIZE - ARRAY_SIZE(bufs));
	zassert_equal(sys_le16_to_cpu(vq.avail->idx), 1U);

	head = sys_le16_to_cpu(vq.avail->ring[0]);
	for (uint16_t i = 0; i < ARRAY_SIZE(bufs); i++) {
		bool last = (sys_le16_to_cpu(vq.desc[head].flags) & VIRTQ_DESC_F_NEXT) == 0U;

		zassert_equal(last, i == ARRAY_SIZE(bufs) - 1, "chain layout differs at %u", i);
		if (!last) {
			head = sys_le16_to_cpu(vq.desc[head].next);
		}
	}

	virtq_free(&vq);
}

ZTEST(virtqueue, test_failed_chain_restores_free_count)
{
	static uint8_t storage[6];
	struct virtq_buf bufs[ARRAY_SIZE(storage)];
	struct virtq vq;

	zassert_ok(virtq_create(&vq, 4U));

	fill_bufs(bufs, ARRAY_SIZE(bufs), storage);
	zassert_equal(virtq_add_buffer_chain(&vq, bufs, ARRAY_SIZE(bufs), 0, NULL, NULL, K_NO_WAIT),
		      -EBUSY);
	zassert_equal(vq.free_desc_n, vq.num);
	zassert_equal(sys_le16_to_cpu(vq.avail->idx), 0U);

	/* The returned descriptors must still be usable for a chain that fits. */
	zassert_ok(virtq_add_buffer_chain(&vq, bufs, vq.num, 0, NULL, NULL, K_NO_WAIT));
	zassert_equal(vq.free_desc_n, 0U);

	virtq_free(&vq);
}

ZTEST(virtqueue, test_isr_completion_returns_whole_chain)
{
	static uint8_t storage[VQ_SIZE];
	struct virtq vq;
	struct virtq_buf bufs[ARRAY_SIZE(storage)];
	const struct device test_virtio_dev = {
		.name = "virtqueue-test",
		.api = &test_virtio_api,
		.data = &vq,
	};
	uint16_t head;

	callback_used_len = 0;
	callback_count = 0;

	zassert_ok(virtq_create(&vq, VQ_SIZE));

	fill_bufs(bufs, 3U, storage);
	zassert_ok(virtq_add_buffer_chain(&vq, bufs, 3U, 0, complete_cb, NULL, K_NO_WAIT));
	head = sys_le16_to_cpu(vq.avail->ring[0]);

	/* Complete the chain from the fake device's used ring. */
	vq.used->ring[0].id = sys_cpu_to_le32(head);
	vq.used->ring[0].len = sys_cpu_to_le32(42U);
	vq.used->idx = sys_cpu_to_le16(1U);

	virtio_isr(&test_virtio_dev, VIRTIO_QUEUE_INTERRUPT, 1U);

	zassert_equal(callback_count, 1U);
	zassert_equal(callback_used_len, 42U);
	zassert_equal(vq.last_used_idx, 1U);
	zassert_equal(vq.free_desc_n, VQ_SIZE);

	/* A full-size chain only fits when the completed descriptors were returned. */
	fill_bufs(bufs, ARRAY_SIZE(bufs), storage);
	zassert_ok(virtq_add_buffer_chain(&vq, bufs, ARRAY_SIZE(bufs), 0, NULL, NULL, K_NO_WAIT));
	zassert_equal(vq.free_desc_n, 0U);

	virtq_free(&vq);
}

ZTEST_SUITE(virtqueue, NULL, NULL, NULL, NULL, NULL);
