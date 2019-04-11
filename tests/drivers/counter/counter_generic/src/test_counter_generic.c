/*
 * Copyright (c) 2018, Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <counter.h>
#include <ztest.h>
#include <kernel.h>
#include <logging/log.h>
#include <entropy.h>

#ifdef CONFIG_SOC_FAMILY_NRF
#include <drivers/clock_control/nrf_clock_control.h>
#include <clock_control.h>
#endif

LOG_MODULE_REGISTER(app, 4);

const char *devices[] = {
#ifdef DT_COUNTER_GENERIC_0_LABEL
	DT_COUNTER_GENERIC_0_LABEL,
#endif

#ifdef DT_COUNTER_GENERIC_1_LABEL
	DT_COUNTER_GENERIC_1_LABEL,
#endif
};

typedef void (*counter_test_func_t)(const char *dev_name);

static volatile int alarm_cnt;

/* copy of struct from counter_generic.c */
struct counter_generic_data {
	const char *backend_name;
	u8_t prescale;
	LOG_INSTANCE_PTR_DECLARE(log);
};

static u32_t backend_period_us;

extern int counter_generic_init_with_max(struct device *dev, u32_t max);

static const char *backend_name(const char *name)
{
	struct device *dev = device_get_binding(name);
	const struct counter_generic_data *devdata =
			(const struct counter_generic_data *)dev->driver_data;

	return devdata->backend_name;
}

static void setup_instance(const char *name)
{
	struct device *dev = device_get_binding(name);
	const struct counter_generic_data *devdata =
			(const struct counter_generic_data *)dev->driver_data;
	struct device *backend = device_get_binding(devdata->backend_name);
	int err;
	u32_t period = 1;

#ifdef CONFIG_SOC_FAMILY_NRF
	struct device *clock;

	clock = device_get_binding(DT_NORDIC_NRF_CLOCK_0_LABEL "_16M");
	__ASSERT_NO_MSG(clock);
	clock_control_on(clock, NULL);

	clock = device_get_binding(DT_NORDIC_NRF_CLOCK_0_LABEL "_32K");
	__ASSERT_NO_MSG(clock);

	while (clock_control_on(clock, (void *)CLOCK_CONTROL_NRF_K32SRC) != 0) {
		/* empty */
	}
#endif
	/* Overwrite default top value to verify that wrapping is handled
	 * correctly.
	 */
	counter_cancel_channel_alarm(backend, 0);
	counter_stop(dev);

	if (counter_get_max_top_value(backend) < UINT32_MAX) {
		/* Set period to be at least 30ms long. */
		do {
			period <<= 1;
		} while (counter_ticks_to_us(backend, period) < 30000);

		backend_period_us = counter_ticks_to_us(backend, period);
		err = counter_generic_init_with_max(dev, period - 1);
		zassert_true(err == 0,
				"%s (%s): failed to reinitialize device.\n",
				name, backend_name(name));
	} else {
		/* */
		backend_period_us = 32000;
	}

	alarm_cnt = 0;
}

static void teardown_instance(const char *name)
{
	struct device *dev = device_get_binding(name);

	counter_stop(dev);
	k_sleep(400);
}

static void test_all_instances(counter_test_func_t func)
{

	for (int i = 0; i < ARRAY_SIZE(devices); i++) {
		setup_instance(devices[i]);
		func(devices[i]);
		teardown_instance(devices[i]);
	}
}
/**
 * Test performs reading of the counter and compares it against reference.
 * In case of a counter which emulates 32 bit timings are adjusted to ensure
 * that base counter wraps.
 */
static void inst_test_read(const char *name)
{
	struct device *dev = device_get_binding(name);
	u32_t val1;
	u32_t val2;
	u32_t exp_val1;
	u32_t exp_val2;
	u32_t delta = counter_us_to_ticks(dev, 1000);
	u32_t wait1 = 2*backend_period_us;
	u32_t wait2 = 1.5*backend_period_us;

	LOG_INF("initializing");

	zassert_true(counter_read(dev) == 0,
			"%s (%s): unexpected read.\n",
			name, backend_name(name));

	exp_val1 = counter_us_to_ticks(dev, wait1);
	exp_val2 = counter_us_to_ticks(dev, wait2) + exp_val1;

	counter_start(dev);

	k_busy_wait(wait1);
	val1 = counter_read(dev);

	k_busy_wait(wait2);
	val2 = counter_read(dev);

	counter_stop(dev);

	zassert_within(val1, exp_val1, delta,
			"%s (%s): got %d when expected %d.\n",
			name, backend_name(name), val1, exp_val1);
	zassert_within(val2, exp_val2, delta,
			"%s (%s): got %d when expected %d.\n",
			name, backend_name(name), val2, exp_val2);
}

static void test_read(void)
{
	test_all_instances(inst_test_read);
}

static void default_alarm_clbk(struct device *dev, u8_t chan_id, u32_t ticks,
			    void *user_data)
{
	u32_t *read = user_data;
	u32_t now = counter_read(dev);

	/* As teardown is not called when zassert fails we need do it here. */
	if (now < ticks) {
		counter_stop(dev);
	}

	zassert_true(now >= ticks, "Expired earlier %d (now %d)", ticks, now);
	if (read) {
		*read = now;
	}
	alarm_cnt++;
}

/* Test that long alarm (alarm longer than backend period) expires on time. */
static void inst_test_long_alarm(const char *name)
{
	struct device *dev = device_get_binding(name);
	u32_t delta = counter_us_to_ticks(dev, 500);
	u32_t clbk_read;
	int err;

	struct counter_alarm_cfg cfg = {
		.callback = default_alarm_clbk,
		.user_data = &clbk_read,
		.absolute = true,
		.ticks = 1.5*counter_us_to_ticks(dev, backend_period_us)
	};

	err = counter_set_channel_alarm(dev, 0, &cfg);
	zassert_true(err == 0,
			"%s (%s): Failed to set an alarm",
			name, backend_name(name));

	counter_start(dev);
	k_busy_wait(counter_ticks_to_us(dev, cfg.ticks) + 2000);
	counter_stop(dev);

	zassert_true(alarm_cnt == 1,
			"%s (%s): expected alarm", name, backend_name(name));
	zassert_within(clbk_read, cfg.ticks, delta,
			"%s (%s): expected %d alarm, got %d",
			name, backend_name(name), cfg.ticks, clbk_read);
}

static void test_long_alarm(void)
{
	test_all_instances(inst_test_long_alarm);
}

/* Helper function which performs series of requested alarms (x ticks from now).
 * It validates if all scheduled alarms expires at requested time or later.
 */
static void test_x_alarm(const char *name, u32_t value)
{
	struct device *dev = device_get_binding(name);
	int err;
	u32_t clbk_read;
	struct counter_alarm_cfg cfg = {
		.callback = default_alarm_clbk,
		.user_data = &clbk_read,
		.absolute = true
	};

	counter_start(dev);

	for (int i = 0; i < 100; i++) {
		cfg.ticks = counter_read(dev) + value;
		err = counter_set_channel_alarm(dev, 0, &cfg);
		zassert_true(err == 0, "%s (%s): failed to set an alarm",
				name, backend_name(name));

		k_busy_wait(2*counter_ticks_to_us(dev, value));

		if ((alarm_cnt != i + 1) || (clbk_read < cfg.ticks)) {
			counter_stop(dev);
		}
		zassert_equal(alarm_cnt, i + 1,
			"%s (%s): unexpected alarm_cnt: %d (i:%d)",
			name, backend_name(name), alarm_cnt, i);
		zassert_true(clbk_read >= cfg.ticks,
			"%s (%s): expected %d, got %d (i:%d)",
			name, backend_name(name), clbk_read, cfg.ticks, i);
	}
}

/* Test that alarm set to 1 tick from now always expires at expected time or
 * later.
 */
static void inst_test_short_1_alarm(const char *name)
{
	test_x_alarm(name, 1);
}

static void test_short_1_alarm(void)
{
	test_all_instances(inst_test_short_1_alarm);
}

/* Test that alarm set to 2 tick from now always expires at expected time or
 * later.
 */
static void inst_test_short_2_alarm(const char *name)
{
	test_x_alarm(name, 2);
}

static void test_short_2_alarm(void)
{
	test_all_instances(inst_test_short_2_alarm);
}

/* Test that alarm set to expire now (now or shortly in the past) immediately
 * expires.
 */
static void inst_test_past_alarm(const char *name)
{
	struct device *dev = device_get_binding(name);
	int err;
	u32_t clbk_read;

	struct counter_alarm_cfg cfg = {
		.callback = default_alarm_clbk,
		.user_data = &clbk_read,
		.absolute = true
	};

	counter_start(dev);
	k_busy_wait(counter_ticks_to_us(dev, 1));

	cfg.ticks = counter_read(dev);
	err = counter_set_channel_alarm(dev, 0, &cfg);
	zassert_true(err == 0, "%s (%s): failed to set an alarm",
			name, backend_name(name));

	if ((alarm_cnt != 1) || (clbk_read < cfg.ticks)) {
		counter_stop(dev);
	}
	zassert_equal(alarm_cnt, 1, "%s (%s): unexpected alarm_cnt: %d",
			name, backend_name(name), alarm_cnt);
	zassert_true(clbk_read >= cfg.ticks, "%s (%s): expected %d, got %d",
			name, backend_name(name), clbk_read, cfg.ticks);
}

static void test_past_alarm(void)
{
	test_all_instances(inst_test_past_alarm);
}

static u32_t periodic_max;
static u32_t periodic_val;
static u32_t dev_ticks;
static u32_t ref_ticks;
static struct device *refdev;

static void periodic_alarm_clbk(struct device *dev, u8_t chan_id, u32_t ticks,
			    void *user_data)
{
	struct counter_alarm_cfg *cfg = user_data;

	if (ticks < periodic_max) {
		int err;

		if (ticks < periodic_max - 1000) {
			cfg->ticks += periodic_val;
		} else {
			cfg->ticks = periodic_max;
		}
		err = counter_set_channel_alarm(dev, 0, cfg);
		zassert_true(err == 0, "%s (%s): failed to set an alarm",
			dev->config->name, backend_name(dev->config->name));
	} else {
		ref_ticks = counter_read(refdev);
		dev_ticks = counter_read(dev);
		alarm_cnt++;
	}
}

/* Test scheduling periodic alarms (next alarm from callback). Verify that there
 * is no accumulative error. Second counter used as a reference.
 */
static void inst_test_periodic_alarm(const char *name)
{
	struct device *dev = device_get_binding(name);
	u32_t ref_stamp;
	u32_t dev_stamp;
	u32_t test_time_us = 1000000;
	u32_t chunks = 100;
	int err;

	struct counter_alarm_cfg cfg = {
		.callback = periodic_alarm_clbk,
		.user_data = &cfg,
		.absolute = true
	};

	refdev = device_get_binding("TIMER_2");
	__ASSERT_NO_MSG(refdev);
	counter_stop(refdev);

	periodic_max = counter_us_to_ticks(dev, test_time_us);
	periodic_val = periodic_max/chunks;
	cfg.ticks = periodic_val;

	err = counter_set_channel_alarm(dev, 0, &cfg);
	zassert_true(err == 0, "%s (%s): failed to set an alarm",
			name, backend_name(name));

	counter_start(refdev);
	counter_start(dev);

	k_busy_wait(1.01 * test_time_us); /* wait for 1,01s */

	ref_stamp = (u32_t)counter_ticks_to_us(refdev, ref_ticks);
	dev_stamp = (u32_t)counter_ticks_to_us(dev, dev_ticks);

	counter_stop(dev);
	counter_stop(refdev);
	zassert_equal(alarm_cnt, 1, "%s (%s): unexpected alarm_cnt: %d",
			name, backend_name(name), alarm_cnt);
	zassert_within(dev_stamp, ref_stamp,
		30 /* todo arbitrary value */,
		"%s (%s): unexpected timestamp %d (expected: %d)",
		name, backend_name(name), dev_stamp, ref_stamp);
}

static void test_periodic_alarm(void)
{
	test_all_instances(inst_test_periodic_alarm);
}

/* Test setting new alarm after canceling previous alarm. Setting new alarm
 * should occur when previous alarm was supposed to expire.
 */
static void inst_test_close_alarms(const char *name)
{
	struct device *dev = device_get_binding(name);
	struct counter_alarm_cfg cfg = {
		.callback = default_alarm_clbk,
		.user_data = NULL,
		.absolute = true
	};
	int err;
	u32_t t1 = counter_us_to_ticks(dev, 200);
	u32_t t2 = counter_us_to_ticks(dev, 1000);

	counter_start(dev);

	for (int i = 0; i < 2000; i++) {
		cfg.ticks = counter_read(dev) + t1;

		err = counter_set_channel_alarm(dev, 0, &cfg);
		zassert_true(err == 0, "%s (%s): failed to set an alarm",
				name, backend_name(name));

		err = counter_cancel_channel_alarm(dev, 0);
		zassert_true(err == 0, "%s (%s): failed to set an alarm",
				name, backend_name(name));

		k_busy_wait(50 + i/10);

		cfg.ticks = counter_read(dev) + t2;
		err = counter_set_channel_alarm(dev, 0, &cfg);
		zassert_true(err == 0, "%s (%s): failed to set an alarm",
				name, backend_name(name));

		k_busy_wait(counter_ticks_to_us(dev, t2)+50);

		if (alarm_cnt != (i + 1)) {
			counter_stop(dev);
		}
		zassert_equal(alarm_cnt, i + 1,
				"%s (%s): unexpected alarm_cnt: %d (exp: %d)",
				name, backend_name(name), alarm_cnt, i + 1);
	}
}

static void test_close_alarms(void)
{
	test_all_instances(inst_test_close_alarms);
}

void test_main(void)
{
	ztest_test_suite(test_counter,
	ztest_unit_test(test_read),
	ztest_unit_test(test_long_alarm),
	ztest_unit_test(test_short_1_alarm),
	ztest_unit_test(test_short_2_alarm),
	ztest_unit_test(test_past_alarm),
	ztest_unit_test(test_periodic_alarm),
	ztest_unit_test(test_close_alarms)
			 );
	ztest_run_test_suite(test_counter);
}
