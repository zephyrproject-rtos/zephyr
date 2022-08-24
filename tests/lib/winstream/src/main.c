/*
 * Copyright (c) 2021 Golioth, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <time.h>


#include <adsp_memory.h>

/* For winstream buffer size,
 * refer to soc/xtensa/intel_adsp/common/trace_out.c
 */
#define TEST_WINSTREAM_BUFFER_SIZE HP_SRAM_WIN3_SIZE

ZTEST(winstream, test_log_frequent)
{
	uint32_t i = 0;

	/* Below log output has no delay, thus very frequent.
	 * And we output for 2 full winstream buffer size to
	 * ensure a wrap around happens. This way, the test suite
	 * header info will be over-written if it is not extracted
	 * yet. Thus leads to a twister failure because twister
	 * relies on the test suite header info to determine test
	 * pass or fail.
	 * If the winstream is fixed, this case should pass.
	 * This test is only meant for platforms with
	 * CONFIG_WINSTREAM=y. Other platforms will skip it.
	 */
	while (i < TEST_WINSTREAM_BUFFER_SIZE * 2) {
		TC_PRINT("M");
		i++;
	}
	TC_PRINT("\n");

}

ZTEST_SUITE(winstream, NULL, NULL, NULL, NULL, NULL);
