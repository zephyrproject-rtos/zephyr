/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stddef.h>
#include <string.h>

#include <zephyr/device.h>
#include <zephyr/drivers/virtio.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/ztest.h>

#include "virtio_common.h"

#define TEST_QUEUE_SIZE 4U
#define TEST_RING_ENTRIES 257U

struct test_used_ring {
	uint16_t flags;
	uint16_t idx;
	struct virtq_used_elem ring[TEST_RING_ENTRIES];
};

BUILD_ASSERT(offsetof(struct test_used_ring, ring) == offsetof(struct virtq_used, ring));

static struct virtq queue;
static struct virtq_desc descriptors[TEST_RING_ENTRIES];
static struct test_used_ring used_ring;
static struct virtq_receive_callback_entry callbacks[TEST_QUEUE_SIZE];
static stack_data_t free_desc_storage[TEST_QUEUE_SIZE];
static size_t callback_count;
static uint32_t callback_len;

static void assert_next_free_desc(uint16_t expected)
{
	uint16_t descriptor;

	zassert_ok(virtq_get_free_desc(&queue, &descriptor, K_NO_WAIT));
	zassert_equal(descriptor, expected, "wrong descriptor returned to the real free stack");
}

static struct virtq *fake_get_virtqueue(const struct device *dev, uint16_t queue_idx)
{
	zassert_not_null(dev);
	zassert_equal(queue_idx, 0U);
	return &queue;
}

static DEVICE_API(virtio, fake_api) = {
	.get_virtqueue = fake_get_virtqueue,
};

static struct device fake_device = {
	.name = "virtio-used-ring-endianness",
	.api = &fake_api,
};

static void receive_callback(void *opaque, uint32_t used_len)
{
	zassert_equal_ptr(opaque, &queue);
	callback_count++;
	callback_len = used_len;
}

static void set_used_entry(size_t slot, uint32_t id, uint32_t len)
{
	used_ring.ring[slot].id = sys_cpu_to_le32(id);
	used_ring.ring[slot].len = sys_cpu_to_le32(len);
}

static void before(void *fixture)
{
	ARG_UNUSED(fixture);

	memset(&queue, 0, sizeof(queue));
	memset(descriptors, 0, sizeof(descriptors));
	memset(&used_ring, 0, sizeof(used_ring));
	memset(callbacks, 0, sizeof(callbacks));
	callback_count = 0U;
	callback_len = 0U;

	queue.num = TEST_QUEUE_SIZE;
	queue.desc = descriptors;
	queue.used = (struct virtq_used *)&used_ring;
	queue.recv_cbs = callbacks;
	k_stack_init(&queue.free_desc_stack, free_desc_storage, ARRAY_SIZE(free_desc_storage));
}

ZTEST(virtio_used_ring_endianness, test_used_element_id_is_little_endian_32_bit)
{
	const uint32_t expected_len = 0x10203040U;

	used_ring.idx = sys_cpu_to_le16(1U);
	set_used_entry(0U, 1U, expected_len);
	callbacks[1].cb = receive_callback;
	callbacks[1].opaque = &queue;

	virtio_isr(&fake_device, VIRTIO_QUEUE_INTERRUPT, 1U);

	zassert_equal(queue.last_used_idx, 1U);
	zassert_equal(queue.free_desc_n, 1U);
	assert_next_free_desc(1U);
	zassert_equal(callback_count, 1U);
	zassert_equal(callback_len, expected_len);
}

ZTEST(virtio_used_ring_endianness, test_descriptor_chain_fields_are_little_endian)
{
	used_ring.idx = sys_cpu_to_le16(1U);
	set_used_entry(0U, 0U, 0U);
	descriptors[0].flags = sys_cpu_to_le16(VIRTQ_DESC_F_NEXT);
	descriptors[0].next = sys_cpu_to_le16(1U);
	callbacks[0].cb = receive_callback;
	callbacks[0].opaque = &queue;

	virtio_isr(&fake_device, VIRTIO_QUEUE_INTERRUPT, 1U);

	zassert_equal(queue.free_desc_n, 2U);
	assert_next_free_desc(1U);
	assert_next_free_desc(0U);
	zassert_equal(callback_count, 1U);
}

ZTEST(virtio_used_ring_endianness, test_used_ring_index_stays_cpu_endian)
{
	queue.last_used_idx = 1U;
	used_ring.idx = sys_cpu_to_le16(2U);
	set_used_entry(1U, 0U, 0U);
	/* Old code byte-swaps slot 1 to 256 and truncates this ID back to 4. */
	set_used_entry(256U, TEST_QUEUE_SIZE << 16, 0U);
	callbacks[0].cb = receive_callback;
	callbacks[0].opaque = &queue;

	virtio_isr(&fake_device, VIRTIO_QUEUE_INTERRUPT, 1U);

	zassert_equal(queue.last_used_idx, 2U);
	zassert_equal(queue.free_desc_n, 1U);
	assert_next_free_desc(0U);
	zassert_equal(callback_count, 1U);
}

ZTEST_SUITE(virtio_used_ring_endianness, NULL, NULL, before, NULL, NULL);
