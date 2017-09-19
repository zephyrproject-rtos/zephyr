/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * @addtogroup test_aon_basic_operations
 * @{
 * @defgroup t_aon_counter test_aon_counter
 * @brief TestPurpose: verify AON counter works well
 * @details
 * - Test Steps
 *   -# Start AON counter and wait for the register change to propagate.
 *   -# Read the counter value before sleep for a while.
 *   -# Sleep for 10ms and read the counter value again.
 *   -# Compare the two counter values.
 * - Expected Results
 *   -# AON counter runs at 32768Hz, which means the counter will decrease
 *	32768 in one second. While k_sleep(10) will sleep 10ms.
 *
 *      The expected result is that 100 * counter_delta >= 32768.
 * @}
 */

#include "test_aon.h"

static int test_counter(void)
{
	u32_t i = 0;
	u32_t p_val, c_val, delta;
	struct device *aon_counter = device_get_binding(AON_COUNTER);

	if (!aon_counter) {
		TC_PRINT("Cannot get AON Counter device\n");
		return TC_FAIL;
	}

	/* verify counter_start() */
	if (counter_start(aon_counter)) {
		TC_PRINT("Fail to start AON Counter device\n");
		return TC_FAIL;
	}

	/*
	 * The AON counter runs from the RTC clock at 32KHz (rather than
	 * the system clock which is 32MHz) so we need to spin for a few cycles
	 * allow the register change to propagate.
	 */
	k_sleep(10);
	TC_PRINT("Always-on counter started\n");

	/* verify counter_read() */
	for (i = 0; i < 20; i++) {
		p_val = counter_read(aon_counter);
		k_sleep(10); //sleep 10 ms
		c_val = counter_read(aon_counter);
		delta = c_val - p_val;
		TC_PRINT("Counter values: %u, %u (%u)\n", p_val, c_val, delta);

		if (delta * 100 < 32768) {
			TC_PRINT("Counter device fails to work\n");
			return TC_FAIL;
		}
	}
	/*
	 * arduino_101 loader assumes the counter is running.
	 * If the counter is stopped, the next app you flash in
	 * cannot start without a hard reset or power cycle.
	 * So let's leave the counter in running state.
	 */
	return TC_PASS;
}

void test_aon_counter(void)
{
	zassert_true(test_counter() == TC_PASS, NULL);
}
