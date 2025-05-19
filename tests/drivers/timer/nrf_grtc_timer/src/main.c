/*
 * Copyright (c) 2022, Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/ztest.h>
#include <zephyr/drivers/timer/nrf_grtc_timer.h>
#include <zephyr/drivers/counter.h>
#include <zephyr/drivers/timer/system_timer.h>
#include <zephyr/random/random.h>
#include <zephyr/logging/log.h>
#include <zephyr/busy_sim.h>
#include <zephyr/debug/cpu_load.h>
#include <nrfx_grtc.h>
#include <hal/nrf_grtc.h>
LOG_MODULE_REGISTER(test, 1);

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

	uint64_t grtc_start_value = z_nrf_grtc_timer_startup_value_get();
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

		curr_tick += (grtc_start_value / CYC_PER_TICK);
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

		curr_tick += (grtc_start_value / CYC_PER_TICK);
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

enum test_timer_state {
	TIMER_IDLE,
	TIMER_PREPARE,
	TIMER_ACTIVE
};

enum test_ctx {
	TEST_HIGH_PRI,
	TEST_TIMER_CB,
	TEST_THREAD
};

struct test_grtc_timer {
	struct k_timer timer;
	uint32_t ticks;
	uint32_t expire;
	uint32_t start_cnt;
	uint32_t expire_cnt;
	uint32_t abort_cnt;
	uint32_t exp_expire;
	int max_late;
	int min_late;
	int avg_late;
	uint32_t early_cnt;
	enum test_timer_state state;
};

static atomic_t test_active_cnt;
static struct test_grtc_timer timers[8];
static uint32_t test_end;
static k_tid_t test_tid;
static volatile bool test_run;
static uint32_t ctx_cnt[3];
static const char *const ctx_name[] = { "HIGH PRIO ISR", "TIMER CALLBACK", "THREAD" };

static bool stress_test_action(int ctx, int id)
{
	struct test_grtc_timer *timer = &timers[id];

	ctx_cnt[ctx]++;
	if (timer->state == TIMER_ACTIVE) {
		/* Aborting soon to expire timers from higher interrupt priority may lead
		 * to test failures.
		 */
		if (ctx == 0 && (k_timer_remaining_get(&timer->timer) < 5)) {
			return true;
		}

		if (timer->abort_cnt < timer->expire_cnt / 2) {
			bool any_active;

			timer->state = TIMER_PREPARE;
			k_timer_stop(&timer->timer);
			timer->abort_cnt++;
			any_active = atomic_dec(&test_active_cnt) > 1;
			timer->state = TIMER_IDLE;

			return any_active;
		}
	} else if (timer->state == TIMER_IDLE) {
		int ticks = 10 + (sys_rand32_get() & 0x3F);
		k_timeout_t t = K_TICKS(ticks);

		timer->state = TIMER_PREPARE;
		timer->exp_expire =  k_ticks_to_cyc_floor32(sys_clock_tick_get_32() + ticks);
		timer->ticks = ticks;
		k_timer_start(&timer->timer, t, K_NO_WAIT);
		atomic_inc(&test_active_cnt);
		timer->start_cnt++;
		timer->state = TIMER_ACTIVE;
	}

	return true;
}

static void stress_test_actions(int ctx)
{
	uint32_t r = sys_rand32_get();
	int action_cnt = max(r & 0x3, 1);
	int tmr_id = (r >> 8) % ARRAY_SIZE(timers);

	/* Occasionally wake thread context from which timer actions are also executed. */
	if ((((r >> 2) & 0x3) == 0) || test_active_cnt < 2) {
		LOG_DBG("ctx:%d thread wakeup", ctx);
		k_wakeup(test_tid);
	}

	for (int i = 0; i < action_cnt; i++) {
		if (stress_test_action(ctx, tmr_id) == false) {
			stress_test_action(ctx, tmr_id);
		}
	}
}

static void timer_cb(struct k_timer *timer)
{
	struct test_grtc_timer *test_timer = CONTAINER_OF(timer, struct test_grtc_timer, timer);
	uint32_t now = k_cycle_get_32();
	int diff = now - test_timer->exp_expire;

	atomic_dec(&test_active_cnt);
	zassert_true(diff >= 0);
	test_timer->max_late = MAX(diff, test_timer->max_late);
	test_timer->min_late = MIN(diff, test_timer->min_late);

	if (test_timer->expire_cnt == 0) {
		test_timer->avg_late = diff;
	} else {
		test_timer->avg_late = (test_timer->avg_late * test_timer->expire_cnt + diff) /
				(test_timer->expire_cnt + 1);
	}

	test_timer->expire_cnt++;
	test_timer->state = TIMER_IDLE;

	if (test_run) {
		stress_test_actions(TEST_TIMER_CB);
	}
}

static void counter_set(const struct device *dev, struct counter_alarm_cfg *cfg)
{
	int err;
	uint32_t us = 150 + (sys_rand32_get() & 0x3F);

	cfg->ticks = counter_us_to_ticks(dev, us);
	err = counter_set_channel_alarm(dev, 0, cfg);
	zassert_equal(err, 0);
}

static void counter_cb(const struct device *dev, uint8_t chan_id, uint32_t ticks, void *user_data)
{
	struct counter_alarm_cfg *config = user_data;

	if (test_run) {
		stress_test_actions(TEST_HIGH_PRI);
		counter_set(dev, config);
	}
}

static void report_progress(uint32_t start, uint32_t end)
{
	static uint32_t next_report;
	static uint32_t step;
	static uint32_t progress;

	if (next_report == 0) {
		step = (end - start) / 10;
		next_report = start + step;
	}

	if (k_uptime_get_32() > next_report) {
		next_report += step;
		progress += 10;
		printk("%d%%\r", progress);
	}
}

static void grtc_stress_test(bool busy_sim_en)
{
	static struct counter_alarm_cfg alarm_cfg;
#if DT_NODE_EXISTS(DT_NODELABEL(test_timer)) && DT_NODE_HAS_STATUS(DT_NODELABEL(test_timer), okay)
	const struct device *const counter_dev = DEVICE_DT_GET(DT_NODELABEL(test_timer));
#else
	const struct device *const counter_dev = NULL;
#endif
	uint32_t test_ms = 5000;
	uint32_t test_start = k_uptime_get_32();
	uint32_t load;

	test_end = k_cycle_get_32() + k_ms_to_cyc_floor32(test_ms);
	test_tid = k_current_get();

	for (size_t i = 0; i < ARRAY_SIZE(timers); i++) {
		k_timer_init(&timers[i].timer, timer_cb, NULL);
	}

	if (IS_ENABLED(CONFIG_CPU_LOAD)) {
		(void)cpu_load_get(true);
	}

	if (counter_dev) {
		counter_start(counter_dev);
	}

	alarm_cfg.callback = counter_cb;
	alarm_cfg.user_data = &alarm_cfg;
	test_run = true;

	if (counter_dev) {
		counter_set(counter_dev, &alarm_cfg);
	}

	if (busy_sim_en) {
		busy_sim_start(500, 200, 1000, 400, NULL);
	}

	LOG_DBG("Starting test, will end at %d", test_end);
	while (k_cycle_get_32() < test_end) {
		report_progress(test_start, test_start + test_ms);
		stress_test_actions(TEST_THREAD);
		k_sleep(K_MSEC(test_ms));
	}

	load = IS_ENABLED(CONFIG_CPU_LOAD) ? cpu_load_get(true) : 0;

	test_run = false;
	k_msleep(50);

	for (size_t i = 0; i < ARRAY_SIZE(timers); i++) {
		zassert_equal(timers[i].state, TIMER_IDLE, "Unexpected timer %d state:%d",
				i, timers[i].state);
		TC_PRINT("Timer%d (%p)\r\n\tstart_cnt:%d abort_cnt:%d expire_cnt:%d\n",
			i, &timers[i], timers[i].start_cnt, timers[i].abort_cnt,
			timers[i].expire_cnt);
		TC_PRINT("\tavarage late:%d ticks, max late:%d, min late:%d early:%d\n",
				timers[i].avg_late, timers[i].max_late, timers[i].min_late,
				timers[i].early_cnt);
	}

	for (size_t i = 0; i < ARRAY_SIZE(ctx_cnt); i++) {
		TC_PRINT("Context: %s executed %d times\n", ctx_name[i], ctx_cnt[i]);
	}
	TC_PRINT("CPU load during test:%d.%d\n", load / 10, load % 10);

	if (busy_sim_en) {
		busy_sim_stop();
	}

	if (counter_dev) {
		counter_stop(counter_dev);
	}
}

ZTEST(nrf_grtc_timer, test_stress)
{
	grtc_stress_test(false);
}

ZTEST_SUITE(nrf_grtc_timer, NULL, NULL, NULL, NULL, NULL);
