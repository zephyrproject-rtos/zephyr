/*
 * Copyright (c) 2020, Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <ztest.h>
#include <drivers/timer/nrf_rtc_timer.h>
#include <hal/nrf_timer.h>
#include <irq.h>

struct test_data {
	uint32_t cc_val;
	uint32_t window;
	uint32_t delay;
	int err;
};

static int timeout_handler_cnt;

ISR_DIRECT_DECLARE(timer0_isr_wrapper)
{
	nrf_timer_event_clear(NRF_TIMER0, NRF_TIMER_EVENT_COMPARE0);

	k_busy_wait(60);

	return 0;
}

static void init_zli_timer0(void)
{
	nrf_timer_mode_set(NRF_TIMER0, NRF_TIMER_MODE_TIMER);
	nrf_timer_bit_width_set(NRF_TIMER0, NRF_TIMER_BIT_WIDTH_32);
	nrf_timer_frequency_set(NRF_TIMER0, NRF_TIMER_FREQ_1MHz);
	nrf_timer_cc_set(NRF_TIMER0, 0, 100);
	nrf_timer_int_enable(NRF_TIMER0, NRF_TIMER_INT_COMPARE0_MASK);
	nrf_timer_shorts_enable(NRF_TIMER0,
				NRF_TIMER_SHORT_COMPARE0_CLEAR_MASK);

	IRQ_DIRECT_CONNECT(TIMER0_IRQn, 0,
			   timer0_isr_wrapper,
			   COND_CODE_1(CONFIG_ZERO_LATENCY_IRQS,
				   (IRQ_ZERO_LATENCY), (0)));
	irq_enable(TIMER0_IRQn);
}

static void start_zli_timer0(void)
{
	nrf_timer_task_trigger(NRF_TIMER0, NRF_TIMER_TASK_START);
}

static void stop_zli_timer0(void)
{
	nrf_timer_task_trigger(NRF_TIMER0, NRF_TIMER_TASK_STOP);
}

static void timeout_handler(uint32_t id, uint32_t cc_value, void *user_data)
{
	struct test_data *data = user_data;
	uint32_t now = z_nrf_rtc_timer_read();
	uint32_t diff = (now - cc_value) & 0x00FFFFFF;

	zassert_true(diff <= data->delay,
		"Handler called in wrong time (%d), set cc: %d, got cc: %d",
		now, data->cc_val, cc_value);

	if ((cc_value >= data->cc_val) &&
	    (cc_value <= (data->cc_val + data->window))) {
		data->err = 0;
	}
	timeout_handler_cnt++;
}

static void test_timeout(uint32_t chan, k_timeout_t t, bool ext_window)
{
	int32_t cc_val = z_nrf_rtc_timer_get_ticks(t);
	struct test_data test_data = {
		.cc_val = cc_val,
		.window = ext_window ? 100 : (Z_TICK_ABS(t.ticks) ? 0 : 32),
		.delay = ext_window ? 100 : 2,
		.err = -EINVAL
	};

	z_nrf_rtc_timer_compare_set(chan, cc_val, timeout_handler, &test_data);

	/* wait additional arbitrary time. */
	k_busy_wait(1000);
	k_sleep(t);

	zassert_equal(test_data.err, 0, "Unexpected err: %d", test_data.err);
}


static void test_basic(void)
{
	int chan = z_nrf_rtc_timer_chan_alloc();

	zassert_true(chan >= 0, "Failed to allocate RTC channel (%d).", chan);

	k_timeout_t t0 =
		Z_TIMEOUT_TICKS(Z_TICK_ABS(z_tick_get() + K_MSEC(1).ticks));

	test_timeout(chan, t0, false);

	k_timeout_t t1 = K_MSEC(4);

	test_timeout(chan, t1, false);


	k_timeout_t t2 = K_MSEC(100);

	test_timeout(chan, t2, false);

	/* value in the past should expire immediately (2 ticks from now)*/
	k_timeout_t t3 =
		Z_TIMEOUT_TICKS(Z_TICK_ABS(z_tick_get() - K_MSEC(1).ticks));

	test_timeout(chan, t3, true);

	z_nrf_rtc_timer_chan_free(chan);
}

static void test_z_nrf_rtc_timer_compare_evt_address_get(void)
{
	uint32_t evt_addr;

	evt_addr = z_nrf_rtc_timer_compare_evt_address_get(0);
	zassert_equal(evt_addr, (uint32_t)&NRF_RTC1->EVENTS_COMPARE[0],
			"Unexpected event addr:%x", evt_addr);
}

static void test_int_disable_enabled(void)
{
	uint32_t now = z_nrf_rtc_timer_read();
	uint32_t t = 1000;
	struct test_data data = {
		.cc_val = now + t,
		.window = 1000,
		.delay = 2000,
		.err = -EINVAL
	};
	bool key;
	int chan;

	chan = z_nrf_rtc_timer_chan_alloc();
	zassert_true(chan >= 0, "Failed to allocate RTC channel.");

	z_nrf_rtc_timer_compare_set(chan, data.cc_val, timeout_handler, &data);

	zassert_equal(data.err, -EINVAL, "Unexpected err: %d", data.err);
	key = z_nrf_rtc_timer_compare_int_lock(chan);

	k_sleep(Z_TIMEOUT_TICKS(t + 100));
	/* No event yet. */
	zassert_equal(data.err, -EINVAL, "Unexpected err: %d", data.err);

	z_nrf_rtc_timer_compare_int_unlock(chan, key);
	k_busy_wait(100);
	zassert_equal(data.err, 0, "Unexpected err: %d", data.err);

	z_nrf_rtc_timer_chan_free(chan);
}

static void test_get_ticks(void)
{
	k_timeout_t t = K_MSEC(1);
	uint32_t exp_ticks = z_nrf_rtc_timer_read() + t.ticks;
	int ticks;

	/* Relative 1ms from now timeout converted to RTC */
	ticks = z_nrf_rtc_timer_get_ticks(t);
	zassert_true((ticks >= exp_ticks) && (ticks <= (exp_ticks + 1)),
		     "Unexpected result %d (expected: %d)", ticks, exp_ticks);

	/* Absolute timeout 1ms in the past */
	t = Z_TIMEOUT_TICKS(Z_TICK_ABS(z_tick_get() - K_MSEC(1).ticks));
	exp_ticks = z_nrf_rtc_timer_read() - K_MSEC(1).ticks;
	ticks = z_nrf_rtc_timer_get_ticks(t);
	zassert_true((ticks >= exp_ticks - 1) && (ticks <= exp_ticks),
		     "Unexpected result %d (expected: %d)", ticks, exp_ticks);

	/* Absolute timeout 10ms in the future */
	t = Z_TIMEOUT_TICKS(Z_TICK_ABS(z_tick_get() + K_MSEC(10).ticks));
	exp_ticks = z_nrf_rtc_timer_read() + K_MSEC(10).ticks;
	ticks = z_nrf_rtc_timer_get_ticks(t);
	zassert_true((ticks >= exp_ticks - 1) && (ticks <= exp_ticks),
		     "Unexpected result %d (expected: %d)", ticks, exp_ticks);

	/* too far in the future */
	t = Z_TIMEOUT_TICKS(z_tick_get() + 0x00800001);
	ticks = z_nrf_rtc_timer_get_ticks(t);
	zassert_equal(ticks, -EINVAL, "Unexpected ticks: %d", ticks);
}


static void sched_handler(uint32_t id, uint32_t cc_val, void *user_data)
{
	int64_t now = z_tick_get();
	int rtc_ticks_now =
	     z_nrf_rtc_timer_get_ticks(Z_TIMEOUT_TICKS(Z_TICK_ABS(now)));
	uint64_t *evt_uptime_us = user_data;

	*evt_uptime_us = k_ticks_to_us_floor64(now - (rtc_ticks_now - cc_val));
}

static void test_absolute_scheduling(void)
{
	k_timeout_t t;
	int64_t now_us = k_ticks_to_us_floor64(z_tick_get());
	uint64_t target_us = now_us + 5678;
	uint64_t evt_uptime_us;
	int rtc_ticks;
	int chan;

	chan = z_nrf_rtc_timer_chan_alloc();
	zassert_true(chan >= 0, "Failed to allocate RTC channel.");

	/* schedule event in 5678us from now */
	t = Z_TIMEOUT_TICKS(Z_TICK_ABS(K_USEC(target_us).ticks));
	rtc_ticks = z_nrf_rtc_timer_get_ticks(t);

	z_nrf_rtc_timer_compare_set(chan, rtc_ticks,
				  sched_handler, &evt_uptime_us);

	k_busy_wait(5678);

	PRINT("RTC event scheduled at %dus for %dus,"
	      "event occured at %dus (uptime)\n",
		(uint32_t)now_us, (uint32_t)target_us, (uint32_t)evt_uptime_us);

	/* schedule event now. */
	now_us = k_ticks_to_us_floor64(z_tick_get());
	t = Z_TIMEOUT_TICKS(Z_TICK_ABS(K_USEC(now_us).ticks));
	rtc_ticks = z_nrf_rtc_timer_get_ticks(t);

	z_nrf_rtc_timer_compare_set(chan, rtc_ticks,
				  sched_handler, &evt_uptime_us);

	k_busy_wait(200);

	PRINT("RTC event scheduled now, at %dus,"
	      "event occured at %dus (uptime)\n",
		(uint32_t)now_us, (uint32_t)evt_uptime_us);

	z_nrf_rtc_timer_chan_free(chan);
}

static void test_alloc_free(void)
{
	int chan[CONFIG_NRF_RTC_TIMER_USER_CHAN_COUNT];
	int inv_ch;

	for (int i = 0; i < CONFIG_NRF_RTC_TIMER_USER_CHAN_COUNT; i++) {
		chan[i] = z_nrf_rtc_timer_chan_alloc();
		zassert_true(chan[i] >= 0, "Failed to allocate RTC channel.");
	}

	inv_ch = z_nrf_rtc_timer_chan_alloc();
	zassert_equal(inv_ch, -ENOMEM, "Unexpected return value %d", inv_ch);

	for (int i = 0; i < CONFIG_NRF_RTC_TIMER_USER_CHAN_COUNT; i++) {
		z_nrf_rtc_timer_chan_free(chan[i]);
	}
}

static void test_stress(void)
{
	int x = 0;
	uint32_t start = k_uptime_get_32();
	uint32_t test_time = 5000;
	int chan = z_nrf_rtc_timer_chan_alloc();

	zassert_true(chan >= 0, "Failed to allocate RTC channel.");
	start_zli_timer0();

	do {
		k_timeout_t t = K_USEC(40 + x);

		test_timeout(chan, t, true);
		/* On every iteration modify timeout to randomize it a bit
		 * against fixed zli interrupt pattern.
		 */
		x += 30;
		if (x > 200) {
			x = 0;
		}
	} while ((k_uptime_get_32() - start) < test_time);

	stop_zli_timer0();
	z_nrf_rtc_timer_chan_free(chan);
}

static void test_reseting_cc(void)
{
	uint32_t start = k_uptime_get_32();
	uint32_t test_time = 1000;
	int chan = z_nrf_rtc_timer_chan_alloc();
	int i = 0;
	int cnt = 0;

	timeout_handler_cnt = 0;

	do {
		uint32_t now = z_nrf_rtc_timer_read();
		struct test_data test_data = {
			.cc_val = now + 5,
			.window = 0,
			.delay = 0,
			.err = -EINVAL
		};

		/* Set compare but expect that it will never expire because
		 * it will be later on reset.
		 */
		z_nrf_rtc_timer_compare_set(chan, now + 2,
					    timeout_handler, &test_data);

		/* Arbitrary variable delay to reset CC before expiring first
		 * request but very close.
		 */
		k_busy_wait(i);
		i = (i + 1) % 20;

		z_nrf_rtc_timer_compare_set(chan, now + 5,
					    timeout_handler, &test_data);
		k_busy_wait((5 + 1)*31);
		cnt++;
	} while ((k_uptime_get_32() - start) < test_time);

	zassert_equal(timeout_handler_cnt, cnt,
		      "Unexpected timeout count %d (exp: %d)",
		      timeout_handler_cnt, cnt);
	z_nrf_rtc_timer_chan_free(chan);
}

void test_main(void)
{
	init_zli_timer0();

	ztest_test_suite(test_nrf_rtc_timer,
		ztest_unit_test(test_basic),
		ztest_unit_test(test_z_nrf_rtc_timer_compare_evt_address_get),
		ztest_unit_test(test_int_disable_enabled),
		ztest_unit_test(test_get_ticks),
		ztest_unit_test(test_absolute_scheduling),
		ztest_unit_test(test_alloc_free),
		ztest_unit_test(test_stress),
		ztest_unit_test(test_reseting_cc)
			 );
	ztest_run_test_suite(test_nrf_rtc_timer);
}
