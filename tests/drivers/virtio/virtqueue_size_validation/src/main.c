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

static void expect_invalid_size(size_t size)
{
	struct virtq vq;
	int ret = virtq_create(&vq, size);

	if (ret == 0) {
		virtq_free(&vq);
	}

	zassert_equal(ret, -EINVAL);
}

static void expect_valid_size(size_t size)
{
	struct virtq vq;
	int ret = virtq_create(&vq, size);

	zassert_ok(ret);
	zassert_equal(vq.num, (uint16_t)size);
	virtq_free(&vq);
}

ZTEST(virtqueue_size_validation, test_non_power_of_two_size_is_rejected)
{
	expect_invalid_size(3U);
}

ZTEST(virtqueue_size_validation, test_size_above_split_ring_limit_is_rejected)
{
	expect_invalid_size(BIT(15) + 1U);
}

ZTEST(virtqueue_size_validation, test_power_of_two_size_is_allowed)
{
	expect_valid_size(4U);
}

ZTEST(virtqueue_size_validation, test_zero_size_unused_queue_is_allowed)
{
	expect_valid_size(0U);
}

ZTEST_SUITE(virtqueue_size_validation, NULL, NULL, NULL, NULL, NULL);
