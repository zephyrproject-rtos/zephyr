/*
 * Copyright (c) 2020 Teslabs Engineering S.L.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/zephyr.h>
#include <ztest.h>

/** Buffer size. */
#define BUF_SIZE 64U

/**
 * @brief Helper function to test SDRAM r/w.
 *
 * @param mem SDRAM memory location to be tested.
 */
static void test_sdram_rw(uint32_t *mem)
{
	/* fill memory with number range (0, BUF_SIZE - 1) */
	for (size_t i = 0U; i < BUF_SIZE; i++) {
		mem[i] = i;
	}

	/* check that memory contains written range */
	for (size_t i = 0U; i < BUF_SIZE; i++) {
		zassert_equal(mem[i], i, "Unexpected content");
	}
}

#if DT_NODE_HAS_STATUS(DT_NODELABEL(sdram1), okay)
/** Buffer on SDRAM1. */
__stm32_sdram1_section uint32_t sdram1[BUF_SIZE];

static void test_sdram1_rw(void)
{
	test_sdram_rw(sdram1);
}
#else
static void test_sdram1_rw(void)
{
	ztest_test_skip();
}
#endif

#if DT_NODE_HAS_STATUS(DT_NODELABEL(sdram2), okay)
/** Buffer on SDRAM2. */
__stm32_sdram2_section uint32_t sdram2[BUF_SIZE];

static void test_sdram2_rw(void)
{
	test_sdram_rw(sdram2);
}

#else
static void test_sdram2_rw(void)
{
	ztest_test_skip();
}
#endif

void test_main(void)
{
	ztest_test_suite(stm32_sdram,
			 ztest_unit_test(test_sdram1_rw),
			 ztest_unit_test(test_sdram2_rw));
	ztest_run_test_suite(stm32_sdram);
}
