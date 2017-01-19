/*
 * Copyright (c) 2015 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _samples_include_tc_nano_timeout_common__h_
#define _samples_include_tc_nano_timeout_common__h_

/*
 * SHORT_TIMEOUTS should be the preferred configuration, but they cause a
 * problem with the Jenkins auto-builders for the ARM QEMU. Until this is
 * fixed, do not use them by default.
 */
#define SHORT_TIMEOUTS 0
#if SHORT_TIMEOUTS
	#define TIMEOUT_BASE 10
	#define TIMEOUT_INCREMENT 5
#else
	#define TIMEOUT_BASE 50
	#define TIMEOUT_INCREMENT 25
#endif
#define TIMEOUT(x) (TIMEOUT_BASE + ((x) * TIMEOUT_INCREMENT))
#define TIMEOUT_TWO_INTERVALS TIMEOUT(1)
#define TIMEOUT_TEN_INTERVALS TIMEOUT(9)

/*
 * Verify a timeout is in range, either a diff of 0 or 1 to account for tick
 * boundaries.
 */
static inline int is_timeout_in_range(int32_t orig_ticks, int32_t expected)
{
	int32_t diff = sys_tick_get() - orig_ticks;

#if SHORT_TIMEOUTS
	/*
	 * This should be the real test: however, there is an issue with the
	 * Jenkins auto-builders and QEMU for ARM, where (it seems) if the
	 * builder is overloaded, they do not give enough time to a QEMU instance
	 * so the Zephyr ticker can increment multiple times (so the interrupt
	 * handling happens) before the regular processing does occur, which gives
	 * the impression that more ticks have elapsed than expected.
	 */

	if (diff != expected && diff != expected + 1) {
		TC_ERROR(" *** timeout skew: expected %d/%d, got %d\n",
					expected, expected + 1, diff);
		return 0;
	}

	/* TC_PRINT("timeout in range (%d vs %d)\n", diff, expected); */
	return 1;
#else
	return diff >= expected;
#endif
}

#endif /* _samples_include_tc_nano_timeout_common__h_ */
