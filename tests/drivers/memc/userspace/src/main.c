/*
 * Copyright (c) 2026 YunHung Hua
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Call the memc API from a user thread on a fake memory-mapped device. memc_read() and
 * memc_write() copy straight from the mapped base plus addr, so besides the device and the user
 * buffer the syscall handlers have to check the range against the device size.
 */

#include <errno.h>
#include <stdint.h>
#include <string.h>

#include <zephyr/device.h>
#include <zephyr/drivers/memc.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>
#include <zephyr/ztest.h>
#include <zephyr/ztest_error_hook.h>

#define MEMC_SIZE 256U

/* The external memory: kernel RAM that a user thread cannot access directly */
static uint8_t backing[MEMC_SIZE];
static const uint8_t memc_id[] = {0x9d, 0x5d};

/* Outside any memory partition, so a user thread cannot pass it as its buffer */
static uint8_t kernel_buf[4];

static void *test_memc_get_mem_base(const struct device *dev)
{
	ARG_UNUSED(dev);

	return backing;
}

static int test_memc_get_size(const struct device *dev, uint64_t *size)
{
	ARG_UNUSED(dev);

	*size = sizeof(backing);

	return 0;
}

static int test_memc_read_id(const struct device *dev, uint8_t *id, size_t len)
{
	ARG_UNUSED(dev);

	memcpy(id, memc_id, MIN(len, sizeof(memc_id)));

	return 0;
}

static DEVICE_API(memc, test_memc_api) = {
	.get_mem_base = test_memc_get_mem_base,
	.get_size = test_memc_get_size,
	.read_id = test_memc_read_id,
};

/* The same memory without get_size, so the handler cannot check a range on it */
static DEVICE_API(memc, test_memc_no_size_api) = {
	.get_mem_base = test_memc_get_mem_base,
};

DEVICE_DEFINE(test_memc, "test_memc", NULL, NULL, NULL, NULL, POST_KERNEL,
	      CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &test_memc_api);
DEVICE_DEFINE(test_memc_no_size, "test_memc_no_size", NULL, NULL, NULL, NULL, POST_KERNEL,
	      CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &test_memc_no_size_api);

static const struct device *const memc = DEVICE_GET(test_memc);
static const struct device *const memc_no_size = DEVICE_GET(test_memc_no_size);

ZTEST_USER(memc_userspace, test_get_size_and_read_id)
{
	uint64_t size = 0;
	uint8_t id[sizeof(memc_id)] = {0};

	zassert_ok(memc_get_size(memc, &size));
	zassert_equal(MEMC_SIZE, size);

	zassert_ok(memc_read_id(memc, id, sizeof(id)));
	zassert_mem_equal(memc_id, id, sizeof(id));
}

ZTEST_USER(memc_userspace, test_in_range_access_allowed)
{
	const uint8_t written[4] = {0x12, 0x34, 0x56, 0x78};
	uint8_t read[4] = {0};

	zassert_ok(memc_write(memc, 0, written, sizeof(written)));
	zassert_ok(memc_read(memc, 0, read, sizeof(read)));
	zassert_mem_equal(written, read, sizeof(read));

	/* The last bytes, where an off-by-one in the bound would show */
	memset(read, 0, sizeof(read));
	zassert_ok(memc_write(memc, MEMC_SIZE - sizeof(written), written, sizeof(written)));
	zassert_ok(memc_read(memc, MEMC_SIZE - sizeof(read), read, sizeof(read)));
	zassert_mem_equal(written, read, sizeof(read));
}

ZTEST_USER(memc_userspace, test_out_of_range_rejected)
{
	uint8_t data[16] = {0};

	/* Starts inside the device and ends past it */
	zassert_equal(-EINVAL, memc_read(memc, MEMC_SIZE - 6U, data, sizeof(data)));
	zassert_equal(-EINVAL, memc_write(memc, MEMC_SIZE - 6U, data, sizeof(data)));

	/* Starts at the end */
	zassert_equal(-EINVAL, memc_read(memc, MEMC_SIZE, data, 1));
	zassert_equal(-EINVAL, memc_write(memc, MEMC_SIZE, data, 1));

	/* Far past the end, and where base + addr wraps to below the device on a 32-bit CPU */
	zassert_equal(-EINVAL, memc_read(memc, 0x100000U, data, 4));
	zassert_equal(-EINVAL, memc_read(memc, UINT32_MAX, data, 2));
	zassert_equal(-EINVAL, memc_write(memc, UINT32_MAX, data, 2));
}

ZTEST_USER(memc_userspace, test_unknown_size_refused)
{
	uint64_t size = 0;
	uint8_t data[4] = {0};

	zassert_equal(-ENOTSUP, memc_get_size(memc_no_size, &size));
	zassert_equal(-ENOTSUP, memc_read(memc_no_size, 0, data, sizeof(data)));
	zassert_equal(-ENOTSUP, memc_write(memc_no_size, 0, data, sizeof(data)));
}

ZTEST_USER(memc_userspace, test_read_into_kernel_buffer_oops)
{
	ztest_set_fault_valid(true);
	(void)memc_read(memc, 0, kernel_buf, sizeof(kernel_buf));

	ztest_set_fault_valid(false);
	zassert_unreachable("memc_read() wrote to kernel memory for a user thread");
}

ZTEST_USER(memc_userspace, test_write_from_kernel_buffer_oops)
{
	ztest_set_fault_valid(true);
	(void)memc_write(memc, 0, kernel_buf, sizeof(kernel_buf));

	ztest_set_fault_valid(false);
	zassert_unreachable("memc_write() read kernel memory for a user thread");
}

static void *setup(void)
{
	k_object_access_grant(memc, k_current_get());
	k_object_access_grant(memc_no_size, k_current_get());

	return NULL;
}

ZTEST_SUITE(memc_userspace, NULL, setup, NULL, NULL, NULL);
