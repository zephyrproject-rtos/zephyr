/*
 * Copyright Meta Platforms, Inc. and its affiliates.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/coredump.h>
#include <ztest.h>

/* Test will verify that these values are present in the core dump */
#define TEST_MEMORY_VALUE_0 0xabababab
#define TEST_MEMORY_VALUE_1 0xcdcdcdcd
#define TEST_MEMORY_VALUE_2 0xefefefef

#if defined(CONFIG_BOARD_QEMU_RISCV32)
#define TEST_MEMORY_VALUE_3 0x12121212
#define TEST_MEMORY_VALUE_4 0x34343434
#define TEST_MEMORY_VALUE_5 0x56565656
#define TEST_MEMORY_VALUE_6 0x78787878
#define TEST_MEMORY_VALUE_7 0x90909090
#endif

#define TEST_MEMORY_VALUE_8 0xbabababa

static uint32_t values_to_dump[3];
static struct coredump_mem_region_node dump_region0 = {
	.start = (uintptr_t)&values_to_dump,
	.size = sizeof(values_to_dump)
};

static void test_coredump_callback(uintptr_t dump_area, size_t dump_area_size)
{
	uint32_t expected_size = DT_PROP_BY_IDX(DT_NODELABEL(coredump_devicecb), memory_regions, 1);

	zassert_equal(dump_area_size, expected_size, "Size in callback doesn't match device tree");
	zassert_not_null((void *)dump_area, "dump_area is NULL");

	uint32_t *mem = (uint32_t *)dump_area;
	*mem = TEST_MEMORY_VALUE_8;
}

static void *coredump_tests_suite_setup(void)
{
#if defined(CONFIG_BOARD_QEMU_RISCV32)
	/* Get addresses of memory regions specified in device tree to fill with test data */
	uint32_t *mem0 =
		(uint32_t *)DT_PROP_BY_IDX(DT_NODELABEL(coredump_device0), memory_regions, 0);
	uint32_t *mem1 =
		(uint32_t *)DT_PROP_BY_IDX(DT_NODELABEL(coredump_device0), memory_regions, 2);
	uint32_t *mem2 =
		(uint32_t *)DT_PROP_BY_IDX(DT_NODELABEL(coredump_device1), memory_regions, 0);

	*mem0 = TEST_MEMORY_VALUE_3;
	*mem1 = TEST_MEMORY_VALUE_4;
	mem2[0] = TEST_MEMORY_VALUE_5;
	mem2[1] = TEST_MEMORY_VALUE_6;
	mem2[2] = TEST_MEMORY_VALUE_7;
#endif

	return NULL;
}

ZTEST_SUITE(coredump_tests, NULL, coredump_tests_suite_setup, NULL, NULL, NULL);

ZTEST(coredump_tests, test_register_memory)
{
	const struct device *coredump_dev = DEVICE_DT_GET(DT_NODELABEL(coredump_device0));
	const struct device *coredump_cb_dev = DEVICE_DT_GET(DT_NODELABEL(coredump_devicecb));

	zassert_not_null(coredump_dev, "Cannot get coredump device");

	/* Verify register callback fails for COREDUMP_TYPE_MEMCPY type device */
	zassert_false(coredump_device_register_callback(coredump_dev, test_coredump_callback),
		"register callback unexpected succeeded");

	/* Verify unregister fails for memory that was never registered */
	zassert_false(coredump_device_unregister_memory(coredump_dev, &dump_region0),
		"unregister unexpected succeeded");

	/* Verify unregister succeeds after registration */
	zassert_true(coredump_device_register_memory(coredump_dev, &dump_region0),
		"register failed");
	zassert_true(coredump_device_unregister_memory(coredump_dev, &dump_region0),
		"unregister failed");

	/* Register dump_region0 to be collected in core dump and set test values */
	zassert_true(coredump_device_register_memory(coredump_dev, &dump_region0),
		"register failed");
	values_to_dump[0] = TEST_MEMORY_VALUE_0;
	values_to_dump[1] = TEST_MEMORY_VALUE_1;
	values_to_dump[2] = TEST_MEMORY_VALUE_2;

	/* Verify register memory region fails for COREDUMP_TYPE_CALLBACK type device */
	zassert_false(coredump_device_register_memory(coredump_cb_dev, &dump_region0),
		"register memory unexpected succeeded");

	/* Register callback to be invoked for the COREDUMP_TYPE_CALLBACK type device  */
	zassert_true(coredump_device_register_callback(coredump_cb_dev, test_coredump_callback),
		"register failed");

	/* Force a crash */
	k_panic();
}
