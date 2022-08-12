/*
 * Copyright (c) 2021 Golioth, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <time.h>


#include <adsp_memory.h>

/* The TEST_OUTPUT_SIZE is chosen based on the output ring buffer size,
 * refer to soc/xtensa/intel_adsp/common/trace_out.c
 */
#define TEST_OUTPUT_SIZE HP_SRAM_WIN3_SIZE

ZTEST(fast_console_output, test_long_fast_console_output)
{
	uint32_t i = 0;
	/* Below console output has no delay, thus very frequent.
	 * And we output for a buffer size that can trigger
	 * a wrap around with current ring buffer design.
	 * Should the console output design change, the test
	 * should still pass.
	 */
	while (i < TEST_OUTPUT_SIZE * 2) {
		TC_PRINT("M");
		i++;
	}
	TC_PRINT("\n");

}

ZTEST_SUITE(fast_console_output, NULL, NULL, NULL, NULL, NULL);
