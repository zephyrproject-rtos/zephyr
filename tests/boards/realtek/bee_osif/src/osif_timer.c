/*
 * Copyright (c) 2018 Intel Corporation
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * Implementation based on: tests/subsys/portability/cmsis_rtos_v2/src/timer.c
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <os_timer.h>
#include <os_sched.h>

#define ONESHOT_TIME_MS 200
#define PERIOD_MS       100
#define NUM_PERIODS     5

#define TIMER_ID1 1
#define TIMER_ID2 2

uint32_t num_oneshots_executed;
uint32_t num_periods_executed;

void Timer1_Callback(void *arg)
{
	uint32_t timer_id;

	os_timer_id_get(&arg, &timer_id);
	zassert_equal(timer_id, TIMER_ID1, "Error getting Timer ID!");

	num_oneshots_executed++;
	TC_PRINT("oneshot_callback (Timer %d) = %d\n", timer_id, num_oneshots_executed);
}

void Timer2_Callback(void *arg)
{
	uint32_t timer_id;

	os_timer_id_get(&arg, &timer_id);
	zassert_equal(timer_id, TIMER_ID2, "Error getting Timer ID!");

	num_periods_executed++;
	TC_PRINT("periodic_callback (Timer %d) = %d\n", timer_id, num_periods_executed);
}

ZTEST(osif_timer, test_timer)
{
	void *timer1 = NULL;
	void *timer2 = NULL;
	bool status;
	uint32_t state;

	/* Create one-shot timer */
	status = os_timer_create(&timer1, "timer1", TIMER_ID1, ONESHOT_TIME_MS, false,
				 Timer1_Callback);
	zassert_true(status != false, "error creating one-shot timer");

	status = os_timer_is_timer_active(&timer1);
	zassert_true(status == false, "timer1 should be inactive!");

	/* Stop the timer before start */
	status = os_timer_stop(&timer1);
	zassert_true(status == false, "error while stopping non-active timer");

	status = os_timer_start(&timer1);
	zassert_true(status == true, "error starting one-shot timer");

	os_timer_state_get(&timer1, &state);
	zassert_equal(state, 1, "Error: Timer not running");

	status = os_timer_is_timer_active(&timer1);
	zassert_true(status == true, "timer1 should be active!");

	/* Timer should fire only once if setup in one shot
	 * mode. Wait for 3 times the one-shot time to see
	 * if it fires more than once.
	 */
	os_delay(ONESHOT_TIME_MS * 3U + 10);
	zassert_true(num_oneshots_executed == 1U,
		     "error setting up one-shot timer (using os_timer_start)");

	status = os_timer_stop(&timer1);
	zassert_true(status == true, "error stopping one-shot timer");

	status = os_timer_restart(&timer1, ONESHOT_TIME_MS * 2);
	zassert_true(status == true, "error restarting one-shot timer");

	os_delay(ONESHOT_TIME_MS * 6U + 15);
	zassert_true(num_oneshots_executed == 2U,
		     "error setting up one-shot timer (using os_timer_restart)");

	status = os_timer_delete(&timer1);
	zassert_true(status == true, "error deleting one-shot timer");

	/* Create periodic timer */
	status = os_timer_create(&timer2, "timer2", TIMER_ID2, PERIOD_MS, true, Timer2_Callback);
	zassert_true(status != false, "error creating periodic timer");

	os_timer_state_get(&timer2, &state);
	zassert_equal(state, 0, "Error: Timer is running");

	status = os_timer_start(&timer2);
	zassert_true(status == true, "error starting periodic timer");

	/* Timer should fire periodically if setup in periodic
	 * mode. Wait for NUM_PERIODS periods to see if it is
	 * fired NUM_PERIODS times.
	 */
	os_delay(PERIOD_MS * NUM_PERIODS + 10);

	zassert_true(num_periods_executed == NUM_PERIODS,
		     "error setting up periodic timer (using os_timer_start)");

	status = os_timer_restart(&timer2, PERIOD_MS * 2);
	zassert_true(status == true, "error starting periodic timer");

	os_delay(PERIOD_MS * NUM_PERIODS * 2 + 10);
	zassert_true(num_periods_executed == NUM_PERIODS * 2,
		     "error setting up periodic timer (using os_timer_restart)");

	/* Delete the timer before stop */
	status = os_timer_delete(&timer2);
	zassert_true(status == true, "error deleting periodic timer");
}
ZTEST_SUITE(osif_timer, NULL, NULL, NULL, NULL, NULL);
