/*
 * Copyright (c) 2026 EPAM Systems
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <string.h>

#include <zephyr/sys/boot_fdt.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/ztest.h>

#define FDT_MAGIC 0xd00dfeed
#define TEST_FDT_SIZE 32U

static uint8_t source_fdt[TEST_FDT_SIZE] __aligned(8);
static uint8_t expected_fdt[TEST_FDT_SIZE] __aligned(8);
static uint8_t copied_fdt[TEST_FDT_SIZE] __aligned(8);

static void prepare_test_fdt(uint32_t magic, uint32_t size)
{
	for (size_t i = 0; i < sizeof(source_fdt); i++) {
		source_fdt[i] = (uint8_t)(0xa0U + i);
	}

	sys_put_be32(magic, source_fdt);
	sys_put_be32(size, source_fdt + sizeof(uint32_t));
	memcpy(expected_fdt, source_fdt, sizeof(expected_fdt));
	memset(copied_fdt, 0, sizeof(copied_fdt));
}

ZTEST(boot_fdt, test_sys_boot_fdt_copy)
{
	uint32_t copied_size;
	int ret;

	prepare_test_fdt(FDT_MAGIC, TEST_FDT_SIZE);

	ret = sys_boot_fdt_copy(copied_fdt, sizeof(copied_fdt), source_fdt,
				&copied_size);

	zassert_ok(ret);
	zassert_equal(copied_size, TEST_FDT_SIZE);
	zassert_mem_equal(copied_fdt, expected_fdt, TEST_FDT_SIZE);

	memset(source_fdt, 0, sizeof(source_fdt));
	zassert_mem_equal(copied_fdt, expected_fdt, TEST_FDT_SIZE);
}

ZTEST(boot_fdt, test_sys_boot_fdt_copy_rejects_invalid_inputs)
{
	uint32_t copied_size;
	int ret;

	prepare_test_fdt(FDT_MAGIC, TEST_FDT_SIZE);

	zassert_equal(sys_boot_fdt_copy(NULL, sizeof(copied_fdt), source_fdt,
					&copied_size), -EINVAL);
	zassert_equal(sys_boot_fdt_copy(copied_fdt, sizeof(copied_fdt), NULL,
					&copied_size), -EINVAL);
	zassert_equal(sys_boot_fdt_copy(copied_fdt, sizeof(copied_fdt),
					source_fdt, NULL), -EINVAL);

	prepare_test_fdt(0U, TEST_FDT_SIZE);
	zassert_equal(sys_boot_fdt_copy(copied_fdt, sizeof(copied_fdt),
					source_fdt, &copied_size), -EINVAL);

	prepare_test_fdt(FDT_MAGIC, sizeof(uint32_t));
	zassert_equal(sys_boot_fdt_copy(copied_fdt, sizeof(copied_fdt),
					source_fdt, &copied_size), -EINVAL);

	prepare_test_fdt(FDT_MAGIC, TEST_FDT_SIZE);
	ret = sys_boot_fdt_copy(copied_fdt, TEST_FDT_SIZE - 1U, source_fdt,
				&copied_size);
	zassert_equal(ret, -ENOSPC);
	zassert_equal(copied_size, TEST_FDT_SIZE);
}

ZTEST_SUITE(boot_fdt, NULL, NULL, NULL, NULL, NULL);
