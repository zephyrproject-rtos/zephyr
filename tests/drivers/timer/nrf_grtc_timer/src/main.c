/*
 * Copyright (c) 2022, Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/ztest.h>
#include <zephyr/drivers/timer/nrf_grtc_timer.h>
#include <hal/nrf_grtc.h>

#define GRTC_SLEW_TICKS 10
#define NUMBER_OF_TRIES 2000
#define CYC_PER_TICK                                                                               \
	((uint64_t)sys_clock_hw_cycles_per_sec() / (uint64_t)CONFIG_SYS_CLOCK_TICKS_PER_SEC)
#define TIMER_COUNT_TIME_MS 10
#define WAIT_FOR_TIMER_EVENT_TIME_MS TIMER_COUNT_TIME_MS + 5

static volatile uint8_t compare_isr_call_counter;

/* GRTC timer compare interrupt handler */
static void timer_compare_interrupt_handler(int32_t id, uint64_t expire_time, void *user_data)
{
	compare_isr_call_counter++;
	TC_PRINT("Compare value reached, user data: '%s'\n", (char *)user_data);
	TC_PRINT("Call counter: %d\n", compare_isr_call_counter);
}

ZTEST(nrf_grtc_timer, test_get_ticks)
{
	k_timeout_t t = K_MSEC(1);

	uint64_t exp_ticks = z_nrf_grtc_timer_read() + t.ticks * CYC_PER_TICK;
	int64_t ticks;

	/* Relative 1ms from now timeout converted to GRTC */
	ticks = z_nrf_grtc_timer_get_ticks(t);
	zassert_true((ticks >= exp_ticks) && (ticks <= (exp_ticks + GRTC_SLEW_TICKS)),
		     "Unexpected result %" PRId64 " (expected: %" PRId64 ")", ticks, exp_ticks);

	k_msleep(1);

	for (uint32_t i = 0; i < NUMBER_OF_TRIES; i++) {
		/* Absolute timeout 1ms in the past */
		uint64_t curr_tick;
		uint64_t curr_grtc_tick;
		uint64_t curr_tick2;

		do {
			/* GRTC and system tick must be read during single system tick. */
			curr_tick = sys_clock_tick_get();
			curr_grtc_tick = z_nrf_grtc_timer_read();
			curr_tick2 = sys_clock_tick_get();
		} while (curr_tick != curr_tick2);

		t = Z_TIMEOUT_TICKS(Z_TICK_ABS(curr_tick - K_MSEC(1).ticks));

		exp_ticks = curr_grtc_tick - K_MSEC(1).ticks * CYC_PER_TICK;
		ticks = z_nrf_grtc_timer_get_ticks(t);

		zassert_true((ticks >= (exp_ticks - CYC_PER_TICK + 1)) &&
				     (ticks <= (exp_ticks + GRTC_SLEW_TICKS)),
			     "Unexpected result %" PRId64 " (expected: %" PRId64 ")", ticks,
			     exp_ticks);

		/* Absolute timeout 10ms in the future */
		do {
			/* GRTC and system tick must be read during single system tick. */
			curr_tick = sys_clock_tick_get();
			curr_grtc_tick = z_nrf_grtc_timer_read();
			curr_tick2 = sys_clock_tick_get();
		} while (curr_tick != curr_tick2);

		t = Z_TIMEOUT_TICKS(Z_TICK_ABS(curr_tick + K_MSEC(10).ticks));
		exp_ticks = curr_grtc_tick + K_MSEC(10).ticks * CYC_PER_TICK;
		ticks = z_nrf_grtc_timer_get_ticks(t);
		zassert_true((ticks >= (exp_ticks - CYC_PER_TICK + 1)) &&
				     (ticks <= (exp_ticks + GRTC_SLEW_TICKS)),
			     "Unexpected result %" PRId64 " (expected: %" PRId64 ")", ticks,
			     exp_ticks);
	}
}

ZTEST(nrf_grtc_timer, test_timer_count_in_compare_mode)
{
	int err;
	uint64_t test_ticks = 0;
	uint64_t compare_value = 0;
	char user_data[] = "test_timer_count_in_compare_mode\n";
	int32_t channel = z_nrf_grtc_timer_chan_alloc();

	TC_PRINT("Allocated GRTC channel %d\n", channel);
	if (channel < 0) {
		TC_PRINT("Failed to allocate GRTC channel, chan=%d\n", channel);
		ztest_test_fail();
	}

	compare_isr_call_counter = 0;
	test_ticks = z_nrf_grtc_timer_get_ticks(K_MSEC(TIMER_COUNT_TIME_MS));
	err = z_nrf_grtc_timer_set(channel, test_ticks, timer_compare_interrupt_handler,
				   (void *)user_data);

	zassert_equal(err, 0, "z_nrf_grtc_timer_set raised an error: %d", err);

	z_nrf_grtc_timer_compare_read(channel, &compare_value);
	zassert_true(K_TIMEOUT_EQ(K_TICKS(compare_value), K_TICKS(test_ticks)),
		     "Compare register set failed");
	zassert_equal(err, 0, "Unexpected error raised when setting timer, err: %d", err);

	k_sleep(K_MSEC(WAIT_FOR_TIMER_EVENT_TIME_MS));

	TC_PRINT("Compare event generated ?: %d\n", z_nrf_grtc_timer_compare_evt_check(channel));
	TC_PRINT("Compare event register address: %X\n",
		 z_nrf_grtc_timer_compare_evt_address_get(channel));

	zassert_equal(compare_isr_call_counter, 1, "Compare isr call counter: %d",
		      compare_isr_call_counter);
	z_nrf_grtc_timer_chan_free(channel);
}

ZTEST(nrf_grtc_timer, test_timer_abort_in_compare_mode)
{
	int err;
	uint64_t test_ticks = 0;
	uint64_t compare_value = 0;
	char user_data[] = "test_timer_abort_in_compare_mode\n";
	int32_t channel = z_nrf_grtc_timer_chan_alloc();

	TC_PRINT("Allocated GRTC channel %d\n", channel);
	if (channel < 0) {
		TC_PRINT("Failed to allocate GRTC channel, chan=%d\n", channel);
		ztest_test_fail();
	}

	compare_isr_call_counter = 0;
	test_ticks = z_nrf_grtc_timer_get_ticks(K_MSEC(TIMER_COUNT_TIME_MS));
	err = z_nrf_grtc_timer_set(channel, test_ticks, timer_compare_interrupt_handler,
				   (void *)user_data);
	zassert_equal(err, 0, "z_nrf_grtc_timer_set raised an error: %d", err);

	z_nrf_grtc_timer_abort(channel);

	z_nrf_grtc_timer_compare_read(channel, &compare_value);
	zassert_true(K_TIMEOUT_EQ(K_TICKS(compare_value), K_TICKS(test_ticks)),
		     "Compare register set failed");

	zassert_equal(err, 0, "Unexpected error raised when setting timer, err: %d", err);

	k_sleep(K_MSEC(WAIT_FOR_TIMER_EVENT_TIME_MS));
	zassert_equal(compare_isr_call_counter, 0, "Compare isr call counter: %d",
		      compare_isr_call_counter);
	z_nrf_grtc_timer_chan_free(channel);
}

ZTEST_SUITE(nrf_grtc_timer, NULL, NULL, NULL, NULL, NULL);
