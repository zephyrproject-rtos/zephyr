/*
 * Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>

#include <zephyr/device.h>
#include <zephyr/drivers/bbram.h>
#include <zephyr/kernel.h>
#include <zephyr/ztest.h>

static const struct device *const dev = DEVICE_DT_GET(DT_NODELABEL(bbram));

/*
 * A user thread picks both offset and size. The drivers bound an access with
 * "offset + size > capacity", so a pair whose sum wraps used to pass that test and reach memory
 * below the BBRAM region. The syscall handler rejects such a range before the driver sees it.
 *
 * The sizes stay small: the handler checks the user buffer before the range, and a huge size
 * would fail that check and take the thread down instead.
 */
ZTEST_USER(bbram_userspace, test_read_wrapping_range_rejected)
{
	uint8_t data[4] = {0};

	zassert_equal(-EFAULT, bbram_read(dev, SIZE_MAX, 1, data));
	zassert_equal(-EFAULT, bbram_read(dev, SIZE_MAX - 3, 4, data));
	zassert_equal(-EFAULT, bbram_read(dev, SIZE_MAX, 2, data));
}

ZTEST_USER(bbram_userspace, test_write_wrapping_range_rejected)
{
	const uint8_t data[4] = {0x5a, 0x5a, 0x5a, 0x5a};

	zassert_equal(-EFAULT, bbram_write(dev, SIZE_MAX, 1, data));
	zassert_equal(-EFAULT, bbram_write(dev, SIZE_MAX - 3, 4, data));
	zassert_equal(-EFAULT, bbram_write(dev, SIZE_MAX, 2, data));
}

ZTEST_USER(bbram_userspace, test_out_of_range_rejected)
{
	size_t capacity = 0;
	uint8_t data[2] = {0};

	zassert_ok(bbram_get_size(dev, &capacity));
	zassert_true(capacity > 1);

	zassert_equal(-EFAULT, bbram_read(dev, capacity, 1, data));
	zassert_equal(-EFAULT, bbram_read(dev, capacity - 1, 2, data));
	zassert_equal(-EFAULT, bbram_read(dev, 0, 0, data));

	zassert_equal(-EFAULT, bbram_write(dev, capacity, 1, data));
	zassert_equal(-EFAULT, bbram_write(dev, capacity - 1, 2, data));
	zassert_equal(-EFAULT, bbram_write(dev, 0, 0, data));
}

ZTEST_USER(bbram_userspace, test_in_range_access_allowed)
{
	size_t capacity = 0;
	uint8_t written = 0xa5;
	uint8_t read = 0;

	zassert_ok(bbram_get_size(dev, &capacity));
	zassert_true(capacity > 0);

	/* First byte, and the last one, which is where the bound is off by one if it is wrong. */
	zassert_ok(bbram_write(dev, 0, 1, &written));
	zassert_ok(bbram_read(dev, 0, 1, &read));
	zassert_equal(written, read);

	written = 0x3c;
	zassert_ok(bbram_write(dev, capacity - 1, 1, &written));
	zassert_ok(bbram_read(dev, capacity - 1, 1, &read));
	zassert_equal(written, read);
}

static void *setup(void)
{
	zassert_true(device_is_ready(dev));
	k_object_access_grant(dev, k_current_get());

	return NULL;
}

ZTEST_SUITE(bbram_userspace, NULL, setup, NULL, NULL, NULL);
