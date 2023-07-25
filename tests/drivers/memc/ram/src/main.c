/*
 * Copyright (c) 2020, Teslabs Engineering S.L.
 * Copyright (c) 2022, Basalte bv
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/linker/devicetree_regions.h>
#include <zephyr/kernel.h>
#include <zephyr/ztest.h>

/** Buffer size. */
#define BUF_SIZE 64U
#define BUF_DEF(label) static uint32_t buf_##label[BUF_SIZE]			\
	Z_GENERIC_SECTION(LINKER_DT_NODE_REGION_NAME(DT_NODELABEL(label)))

/**
 * @brief Helper function to test RAM r/w.
 *
 * @param mem RAM memory location to be tested.
 */
static void test_ram_rw(uint32_t *mem, size_t size)
{
	/* fill memory with number range (0, BUF_SIZE - 1) */
	for (size_t i = 0U; i < size / sizeof(uint32_t); i++) {
		mem[i] = i;
	}

	/* check that memory contains written range */
	for (size_t i = 0U; i < size / sizeof(uint32_t); i++) {
		zassert_equal(mem[i], i, "Unexpected content on byte %ld", i);
	}
}

#if DT_NODE_HAS_STATUS(DT_NODELABEL(sdram1), okay)
BUF_DEF(sdram1);
#endif
#if DT_NODE_HAS_STATUS(DT_NODELABEL(sdram2), okay)
BUF_DEF(sdram2);
#endif
#if DT_NODE_HAS_STATUS(DT_NODELABEL(sram1), okay)
BUF_DEF(sram1);
#endif
#if DT_NODE_HAS_STATUS(DT_NODELABEL(sram2), okay)
BUF_DEF(sram2);
#endif

#if DT_NODE_HAS_STATUS(DT_NODELABEL(ram0), okay)
#define RAM_SIZE DT_REG_SIZE(DT_NODELABEL(ram0))
static uint32_t *buf_ram0 = (uint32_t *)DT_REG_ADDR(DT_NODELABEL(ram0));
#endif

ZTEST_SUITE(test_ram, NULL, NULL, NULL, NULL, NULL);

ZTEST(test_ram, test_sdram1)
{
#if DT_NODE_HAS_STATUS(DT_NODELABEL(sdram1), okay)
	test_ram_rw(buf_sdram1, BUF_SIZE);
#else
	ztest_test_skip();
#endif
}

ZTEST(test_ram, test_ram0)
{
#if DT_NODE_HAS_STATUS(DT_NODELABEL(ram0), okay)
	test_ram_rw(buf_ram0, RAM_SIZE);
#else
	ztest_test_skip();
#endif
}

ZTEST(test_ram, test_sdram2)
{
#if DT_NODE_HAS_STATUS(DT_NODELABEL(sdram2), okay)
	test_ram_rw(buf_sdram2, BUF_SIZE);
#else
	ztest_test_skip();
#endif
}

ZTEST(test_ram, test_sram1)
{
#if DT_NODE_HAS_STATUS(DT_NODELABEL(sram1), okay)
	test_ram_rw(buf_sram1, BUF_SIZE);
#else
	ztest_test_skip();
#endif
}

ZTEST(test_ram, test_sram2)
{
#if DT_NODE_HAS_STATUS(DT_NODELABEL(sram2), okay)
	test_ram_rw(buf_sram2, BUF_SIZE);
#else
	ztest_test_skip();
#endif
}
