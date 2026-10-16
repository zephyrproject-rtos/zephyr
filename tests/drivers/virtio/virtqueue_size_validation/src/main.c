/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stddef.h>

#include <zephyr/drivers/virtio/virtqueue.h>
#include <zephyr/sys/util.h>
#include <zephyr/ztest.h>

#define TEST_VIRTQ_MAX_SIZE 8U

VIRTQ_STORAGE_DEFINE(test_vq, TEST_VIRTQ_MAX_SIZE);
VIRTQ_STORAGE_DEFINE(test_unused_vq, 0);

static struct virtq test_vq = VIRTQ_INITIALIZER(test_vq, TEST_VIRTQ_MAX_SIZE);
static struct virtq test_unused_vq = VIRTQ_INITIALIZER(test_unused_vq, 0);

static void expect_invalid_size(size_t max_size)
{
	zassert_equal(virtq_create(&test_vq, max_size), -EINVAL);
}

static void expect_size(size_t max_size, uint16_t size)
{
	zassert_ok(virtq_create(&test_vq, max_size));
	zassert_equal(test_vq.num, size);
	zassert_equal(test_vq.max_num, TEST_VIRTQ_MAX_SIZE);
	zassert_equal(test_vq.free_desc_n, size);
}

ZTEST(virtqueue_size_validation, test_non_power_of_two_sizes_are_rejected)
{
	expect_invalid_size(3U);
	expect_invalid_size(6U);
	expect_invalid_size(TEST_VIRTQ_MAX_SIZE - 1U);
}

ZTEST(virtqueue_size_validation, test_power_of_two_size_is_allowed)
{
	expect_size(TEST_VIRTQ_MAX_SIZE, TEST_VIRTQ_MAX_SIZE);
	expect_size(4U, 4U);
	expect_size(1U, 1U);
}

ZTEST(virtqueue_size_validation, test_size_is_limited_to_storage)
{
	expect_size(2U * TEST_VIRTQ_MAX_SIZE, TEST_VIRTQ_MAX_SIZE);
	expect_size(TEST_VIRTQ_MAX_SIZE + 1U, TEST_VIRTQ_MAX_SIZE);
	expect_size(KB(32), TEST_VIRTQ_MAX_SIZE);
	expect_size(KB(64), TEST_VIRTQ_MAX_SIZE);
}

ZTEST(virtqueue_size_validation, test_zero_size_unused_queue_is_allowed)
{
	expect_size(0U, 0U);

	zassert_ok(virtq_create(&test_unused_vq, 128U));
	zassert_equal(test_unused_vq.num, 0U);
}

ZTEST(virtqueue_size_validation, test_rings_are_placed_in_storage)
{
	uint8_t *ring_area;

	zassert_ok(virtq_create(&test_vq, TEST_VIRTQ_MAX_SIZE));

	ring_area = (uint8_t *)test_vq.desc;
	zassert_true(IS_ALIGNED(ring_area, 16));
	zassert_true(IS_ALIGNED(test_vq.used, 4));
	zassert_true((uint8_t *)test_vq.avail >= ring_area + 16U * TEST_VIRTQ_MAX_SIZE);
	zassert_true((uint8_t *)test_vq.used >=
		     (uint8_t *)test_vq.avail + 2U * TEST_VIRTQ_MAX_SIZE + 6U);
	zassert_true((uint8_t *)test_vq.used + 8U * TEST_VIRTQ_MAX_SIZE + 6U <=
		     ring_area + VIRTQ_RING_AREA_SIZE(TEST_VIRTQ_MAX_SIZE));
}

ZTEST_SUITE(virtqueue_size_validation, NULL, NULL, NULL, NULL, NULL);
