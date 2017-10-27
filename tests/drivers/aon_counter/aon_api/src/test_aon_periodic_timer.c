/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * @addtogroup test_aon_basic_operations
 * @{
 * @defgroup t_aon_periodic_timer test_aon_periodic_timer
 * @brief TestPurpose: verify AON timer works well
 * @details
 * - Test Steps
 *   -# Start AON timer and wait for the register change to propagate.
 *   -# Set timer to alarm every second.
 *   -# Sleep for a long time to wait for the alarm invoked.
 * - Expected Results
 *   -# AON counter runs at 32768Hz, which means the counter will decrease
 *	32768 in one second. Set AONT down counter initial value register to
 *	32768, so the alarm will be invoked every second.
 *
 *	Sleep for a little longer than 3 seconds, the timer ISR is expected
 *	to be invoked 3 times.
 * @}
 */

#include "test_aon.h"

#define ALARM_CNT 32768 /* about 1s */
#define SLEEP_TIME 3050 /* a little longer than 3s */

static volatile int actual_alarm_cnt;

static void aon_timer_callback(struct device *dev, void *user_data)
{
	TC_PRINT("Periodic timer callback invoked: %u\n", counter_read(dev));

	/* verify counter_get_pending_int() */
	if (counter_get_pending_int(dev)) {
		TC_PRINT("Counter interrupt is pending\n");
		actual_alarm_cnt++;
	}
}

static int test_timer(void)
{
	u32_t dummy_data = 30;
	int expected_alarm_cnt = 0;
	struct device *aon_timer = device_get_binding(AON_TIMER);

	if (!aon_timer) {
		TC_PRINT("Cannot get AON Timer device\n");
		return TC_FAIL;
	}

	/* verify counter_start() */
	if (counter_start(aon_timer)) {
		TC_PRINT("Fail to start AON Timer device\n");
		return TC_FAIL;
	}

	/*
	 * The AON counter runs from the RTC clock at 32KHz (rather than
	 * the system clock which is 32MHz) so we need to spin for a few cycles
	 * allow the register change to propagate.
	 */
	k_sleep(10);
	TC_PRINT("Always-on timer started\n");

	/* verify counter_set_alarm() */
	if (counter_set_alarm(aon_timer, aon_timer_callback,
			      ALARM_CNT, (void *)&dummy_data)) {
		TC_PRINT("Fail to set alarm for AON Timer\n");
		return TC_FAIL;
	}

	/* long delay for the alarm and callback to happen */
	k_sleep(SLEEP_TIME);

	if (counter_set_alarm(aon_timer, NULL, 0, NULL)) {
		TC_PRINT("Periodic timer can not turn off\n");
		return TC_FAIL;
	}

	expected_alarm_cnt = (SLEEP_TIME/1000) / (32768/ALARM_CNT);

	TC_PRINT("expected_alarm_cnt = %d\n", expected_alarm_cnt);
	TC_PRINT("actual_alarm_cnt = %d\n", actual_alarm_cnt);

	if (actual_alarm_cnt != expected_alarm_cnt) {
		TC_PRINT("actual_alarm_cnt doesn't match expected_alarm_cnt\n");
		return TC_FAIL;
	}
	/*
	 * arduino_101 loader assumes the counter is running.
	 * If the counter is stopped, the next app you flash in
	 * cannot start without a hard reset or power cycle.
	 * So let's leave the counter in running state.
	 */
	return TC_PASS;
}

void test_aon_periodic_timer(void)
{
	zassert_true(test_timer() == TC_PASS, NULL);
}
