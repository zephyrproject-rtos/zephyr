/*
 * Copyright (c) 2018, Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <drivers/counter.h>
#include <ztest.h>
#include <kernel.h>
#include <logging/log.h>
LOG_MODULE_REGISTER(test);

static volatile u32_t top_cnt;
static volatile u32_t alarm_cnt;

static void top_handler(struct device *dev, void *user_data);

void *exp_user_data = (void *)199;

#if defined(CONFIG_COUNTER_MCUX_RTC) || defined(CONFIG_COUNTER_RTC_STM32)
#define COUNTER_PERIOD_US (USEC_PER_SEC * 2U)
#else
#define COUNTER_PERIOD_US 20000
#endif

struct counter_alarm_cfg alarm_cfg;
struct counter_alarm_cfg alarm_cfg2;

const char *devices[] = {
#ifdef CONFIG_COUNTER_TIMER0
	/* Nordic TIMER0 may be reserved for Bluetooth */
	DT_NORDIC_NRF_TIMER_TIMER_0_LABEL,
#endif
#ifdef CONFIG_COUNTER_TIMER1
	DT_NORDIC_NRF_TIMER_TIMER_1_LABEL,
#endif
#ifdef CONFIG_COUNTER_TIMER2
	DT_NORDIC_NRF_TIMER_TIMER_2_LABEL,
#endif
#ifdef CONFIG_COUNTER_TIMER3
	DT_NORDIC_NRF_TIMER_TIMER_3_LABEL,
#endif
#ifdef CONFIG_COUNTER_TIMER4
	DT_NORDIC_NRF_TIMER_TIMER_4_LABEL,
#endif
#ifdef CONFIG_COUNTER_RTC0
	/* Nordic RTC0 may be reserved for Bluetooth */
	DT_NORDIC_NRF_RTC_RTC_0_LABEL,
#endif
	/* Nordic RTC1 is used for the system clock */
#ifdef CONFIG_COUNTER_RTC2
	DT_NORDIC_NRF_RTC_RTC_2_LABEL,
#endif
#ifdef CONFIG_COUNTER_IMX_EPIT_1
	DT_COUNTER_IMX_EPIT_1_LABEL,
#endif
#ifdef CONFIG_COUNTER_IMX_EPIT_2
	DT_COUNTER_IMX_EPIT_2_LABEL,
#endif
#ifdef DT_RTC_MCUX_0_NAME
	DT_RTC_MCUX_0_NAME,
#endif
#ifdef DT_INST_0_ARM_CMSDK_TIMER_LABEL
	DT_INST_0_ARM_CMSDK_TIMER_LABEL,
#endif
#ifdef DT_INST_1_ARM_CMSDK_TIMER_LABEL
	DT_INST_1_ARM_CMSDK_TIMER_LABEL,
#endif
#ifdef DT_INST_0_ARM_CMSDK_DTIMER_LABEL
	DT_INST_0_ARM_CMSDK_DTIMER_LABEL,
#endif
#ifdef DT_RTC_0_NAME
	DT_RTC_0_NAME,
#endif

};
typedef void (*counter_test_func_t)(const char *dev_name);

typedef bool (*counter_capability_func_t)(const char *dev_name);


static void counter_setup_instance(const char *dev_name)
{
	alarm_cnt = 0U;
}

static void counter_tear_down_instance(const char *dev_name)
{
	int err;
	struct device *dev;
	struct counter_top_cfg top_cfg = {
		.callback = NULL,
		.user_data = NULL,
		.flags = 0
	};

	dev = device_get_binding(dev_name);

	top_cfg.ticks = counter_get_max_top_value(dev);
	err = counter_set_top_value(dev, &top_cfg);
	zassert_equal(0, err,
			"%s: Setting top value to default failed", dev_name);

	err = counter_stop(dev);
	zassert_equal(0, err, "%s: Counter failed to stop", dev_name);

}

static void test_all_instances(counter_test_func_t func,
				counter_capability_func_t capability_check)
{
	for (int i = 0; i < ARRAY_SIZE(devices); i++) {
		counter_setup_instance(devices[i]);
		if ((capability_check == NULL) ||
		     capability_check(devices[i])) {
			TC_PRINT("Testing %s\n", devices[i]);
			func(devices[i]);
		} else {
			TC_PRINT("Skipped for %s\n", devices[i]);
		}
		counter_tear_down_instance(devices[i]);
		/* Allow logs to be printed. */
		k_sleep(K_MSEC(100));
	}
}

static bool set_top_value_capable(const char *dev_name)
{
	struct device *dev = device_get_binding(dev_name);
	struct counter_top_cfg cfg = {
		.ticks = counter_get_top_value(dev) - 1
	};

	int err = counter_set_top_value(dev, &cfg);

	if (err == -ENOTSUP) {
		return false;
	}

	cfg.ticks++;
	counter_set_top_value(dev, &cfg);
	return true;
}

static void top_handler(struct device *dev, void *user_data)
{
	zassert_true(user_data == exp_user_data,
			"%s: Unexpected callback", dev->config->name);
	top_cnt++;
}

void test_set_top_value_with_alarm_instance(const char *dev_name)
{
	struct device *dev;
	int err;
	u32_t cnt;
	u32_t tmp_top_cnt;
	struct counter_top_cfg top_cfg = {
		.callback = top_handler,
		.user_data = exp_user_data,
		.flags = 0
	};

	top_cnt = 0U;

	dev = device_get_binding(dev_name);
	top_cfg.ticks = counter_us_to_ticks(dev, COUNTER_PERIOD_US);
	err = counter_start(dev);
	zassert_equal(0, err, "%s: Counter failed to start", dev_name);

	k_busy_wait(5000);

	cnt = counter_read(dev);
	zassert_true(cnt > 0, "%s: Counter should progress", dev_name);

	err = counter_set_top_value(dev, &top_cfg);
	zassert_equal(0, err, "%s: Counter failed to set top value (err: %d)",
			dev_name, err);

	k_busy_wait(5.2*COUNTER_PERIOD_US);

	tmp_top_cnt = top_cnt; /* to avoid passing volatile to the macro */
	zassert_true(tmp_top_cnt == 5U,
			"%s: Unexpected number of turnarounds (%d).",
			dev_name, tmp_top_cnt);
}

void test_set_top_value_with_alarm(void)
{
	test_all_instances(test_set_top_value_with_alarm_instance,
			   set_top_value_capable);
}

static void alarm_handler(struct device *dev, u8_t chan_id, u32_t counter,
			  void *user_data)
{
	u32_t now = counter_read(dev);

	zassert_true(now >= counter,
			"%s: Alarm (%d) too early now:%d.",
			dev->config->name, counter, now);

	if (user_data) {
		zassert_true(&alarm_cfg == user_data,
			"%s: Unexpected callback", dev->config->name);
	}

	zassert_true(k_is_in_isr(), "%s: Expected interrupt context",
			dev->config->name);
	alarm_cnt++;
}

void test_single_shot_alarm_instance(const char *dev_name, bool set_top)
{
	struct device *dev;
	int err;
	u32_t ticks;
	u32_t tmp_alarm_cnt;
	struct counter_top_cfg top_cfg = {
		.callback = top_handler,
		.user_data = exp_user_data,
		.flags = 0
	};

	dev = device_get_binding(dev_name);
	ticks = counter_us_to_ticks(dev, COUNTER_PERIOD_US);
	top_cfg.ticks = ticks;

	alarm_cfg.flags = 0;
	alarm_cfg.callback = alarm_handler;
	alarm_cfg.user_data = &alarm_cfg;

	alarm_cnt = 0U;

	if (counter_get_num_of_channels(dev) < 1U) {
		/* Counter does not support any alarm */
		return;
	}

	err = counter_start(dev);
	zassert_equal(0, err, "%s: Counter failed to start", dev_name);

	if (set_top) {
		err = counter_set_top_value(dev, &top_cfg);

		zassert_equal(0, err,
			     "%s: Counter failed to set top value", dev_name);

		alarm_cfg.ticks = ticks + 1;
		err = counter_set_channel_alarm(dev, 0, &alarm_cfg);
		zassert_equal(-EINVAL, err,
			      "%s: Counter should return error because ticks"
			      " exceeded the limit set alarm", dev_name);
		alarm_cfg.ticks = ticks - 1;
	}

	alarm_cfg.ticks = ticks;
	err = counter_set_channel_alarm(dev, 0, &alarm_cfg);
	zassert_equal(0, err, "%s: Counter set alarm failed (err: %d)",
			dev_name, err);

	k_busy_wait(2*(u32_t)counter_ticks_to_us(dev, ticks));

	tmp_alarm_cnt = alarm_cnt; /* to avoid passing volatile to the macro */
	zassert_equal(1, tmp_alarm_cnt,
			"%s: Expecting alarm callback", dev_name);

	k_busy_wait(1.5*counter_ticks_to_us(dev, ticks));
	tmp_alarm_cnt = alarm_cnt; /* to avoid passing volatile to the macro */
	zassert_equal(1, tmp_alarm_cnt,
			"%s: Expecting alarm callback", dev_name);

	err = counter_cancel_channel_alarm(dev, 0);
	zassert_equal(0, err, "%s: Counter disabling alarm failed", dev_name);

	top_cfg.ticks = counter_get_max_top_value(dev);
	top_cfg.callback = NULL;
	top_cfg.user_data = NULL;
	err = counter_set_top_value(dev, &top_cfg);
	zassert_equal(0, err, "%s: Setting top value to default failed",
			dev_name);

	err = counter_stop(dev);
	zassert_equal(0, err, "%s: Counter failed to stop", dev_name);
}

void test_single_shot_alarm_notop_instance(const char *dev_name)
{
	test_single_shot_alarm_instance(dev_name, false);
}

void test_single_shot_alarm_top_instance(const char *dev_name)
{
	test_single_shot_alarm_instance(dev_name, true);
}

static bool single_channel_alarm_capable(const char *dev_name)
{
	struct device *dev = device_get_binding(dev_name);

	return (counter_get_num_of_channels(dev) > 0);
}

static bool single_channel_alarm_and_custom_top_capable(const char *dev_name)
{
	return single_channel_alarm_capable(dev_name) &&
		set_top_value_capable(dev_name);
}

void test_single_shot_alarm_notop(void)
{
	test_all_instances(test_single_shot_alarm_notop_instance,
			   single_channel_alarm_capable);
}

void test_single_shot_alarm_top(void)
{
	test_all_instances(test_single_shot_alarm_top_instance,
			   single_channel_alarm_and_custom_top_capable);
}

static void *clbk_data[10];

static void alarm_handler2(struct device *dev, u8_t chan_id, u32_t counter,
			   void *user_data)
{
	clbk_data[alarm_cnt] = user_data;
	alarm_cnt++;
}

/*
 * Two alarms set. First alarm is absolute, second relative. Because
 * setting of both alarms is delayed it is expected that second alarm
 * will expire first (relative to the time called) while first alarm
 * will expire after next wrap around.
 */
void test_multiple_alarms_instance(const char *dev_name)
{
	struct device *dev;
	int err;
	u32_t ticks;
	u32_t tmp_alarm_cnt;
	struct counter_top_cfg top_cfg = {
		.callback = top_handler,
		.user_data = exp_user_data,
		.flags = 0
	};

	dev = device_get_binding(dev_name);
	ticks = counter_us_to_ticks(dev, COUNTER_PERIOD_US);
	top_cfg.ticks = ticks;

	alarm_cfg.flags = COUNTER_ALARM_CFG_ABSOLUTE;
	alarm_cfg.ticks = counter_us_to_ticks(dev, 2000);
	alarm_cfg.callback = alarm_handler2;
	alarm_cfg.user_data = &alarm_cfg;

	alarm_cfg2.flags = 0;
	alarm_cfg2.ticks = counter_us_to_ticks(dev, 2000);
	alarm_cfg2.callback = alarm_handler2;
	alarm_cfg2.user_data = &alarm_cfg2;

	alarm_cnt = 0U;

	if (counter_get_num_of_channels(dev) < 2U) {
		/* Counter does not support two alarms */
		return;
	}

	err = counter_start(dev);
	zassert_equal(0, err, "%s: Counter failed to start", dev_name);

	err = counter_set_top_value(dev, &top_cfg);
	zassert_equal(0, err,
			"%s: Counter failed to set top value", dev_name);

	k_busy_wait(3*(u32_t)counter_ticks_to_us(dev, alarm_cfg.ticks));

	err = counter_set_channel_alarm(dev, 0, &alarm_cfg);
	zassert_equal(0, err, "%s: Counter set alarm failed", dev_name);

	err = counter_set_channel_alarm(dev, 1, &alarm_cfg2);
	zassert_equal(0, err, "%s: Counter set alarm failed", dev_name);

	k_busy_wait(1.2*counter_ticks_to_us(dev, ticks * 2U));
	tmp_alarm_cnt = alarm_cnt; /* to avoid passing volatile to the macro */
	zassert_equal(2, tmp_alarm_cnt,
			"%s: Invalid number of callbacks %d (expected: %d)",
			dev_name, tmp_alarm_cnt, 2);

	zassert_equal(&alarm_cfg2, clbk_data[0],
			"%s: Expected different order or callbacks",
			dev_name);
	zassert_equal(&alarm_cfg, clbk_data[1],
			"%s: Expected different order or callbacks",
			dev_name);

	/* tear down */
	err = counter_cancel_channel_alarm(dev, 0);
	zassert_equal(0, err, "%s: Counter disabling alarm failed", dev_name);

	err = counter_cancel_channel_alarm(dev, 1);
	zassert_equal(0, err, "%s: Counter disabling alarm failed", dev_name);
}

static bool multiple_channel_alarm_capable(const char *dev_name)
{
	struct device *dev = device_get_binding(dev_name);

	return (counter_get_num_of_channels(dev) > 1);
}

void test_multiple_alarms(void)
{
	test_all_instances(test_multiple_alarms_instance,
			   multiple_channel_alarm_capable);
}

void test_all_channels_instance(const char *dev_name)
{
	struct device *dev;
	int err;
	const int n = 10;
	int nchan = 0;
	bool limit_reached = false;
	struct counter_alarm_cfg alarm_cfgs;
	u32_t ticks;
	u32_t tmp_alarm_cnt;

	dev = device_get_binding(dev_name);
	ticks = counter_us_to_ticks(dev, COUNTER_PERIOD_US);

	alarm_cfgs.flags = 0;
	alarm_cfgs.ticks = ticks;
	alarm_cfgs.callback = alarm_handler2;
	alarm_cfgs.user_data = NULL;

	err = counter_start(dev);
	zassert_equal(0, err, "%s: Counter failed to start", dev_name);

	for (int i = 0; i < n; i++) {
		err = counter_set_channel_alarm(dev, i, &alarm_cfgs);
		if ((err == 0) && !limit_reached) {
			nchan++;
		} else if (err == -ENOTSUP) {
			limit_reached = true;
		} else {
			zassert_equal(0, 1,
			   "%s: Unexpected error on setting alarm", dev_name);
		}
	}

	k_busy_wait(1.5*counter_ticks_to_us(dev, ticks));
	tmp_alarm_cnt = alarm_cnt; /* to avoid passing volatile to the macro */
	zassert_equal(nchan, tmp_alarm_cnt,
			"%s: Expecting alarm callback", dev_name);

	for (int i = 0; i < nchan; i++) {
		err = counter_cancel_channel_alarm(dev, i);
		zassert_equal(0, err,
			"%s: Unexpected error on disabling alarm", dev_name);
	}

	for (int i = nchan; i < n; i++) {
		err = counter_cancel_channel_alarm(dev, i);
		zassert_equal(-ENOTSUP, err,
			"%s: Unexpected error on disabling alarm", dev_name);
	}
}

void test_all_channels(void)
{
	test_all_instances(test_all_channels_instance,
			   single_channel_alarm_capable);
}

/**
 * Test validates if alarm set too late (current tick or current tick + 1)
 * results in callback being called.
 */
void test_late_alarm_instance(const char *dev_name)
{
	int err;
	u32_t tmp_alarm_cnt;
	struct device *dev = device_get_binding(dev_name);
	u32_t tick_us = (u32_t)counter_ticks_to_us(dev, 1);
	u32_t guard = counter_us_to_ticks(dev, 200);
	struct counter_alarm_cfg alarm_cfg = {
		.callback = alarm_handler,
		.flags = COUNTER_ALARM_CFG_ABSOLUTE |
			 COUNTER_ALARM_CFG_EXPIRE_WHEN_LATE,
		.user_data = NULL
	};

	err = counter_set_guard_period(dev, guard,
					COUNTER_GUARD_PERIOD_LATE_TO_SET);
	zassert_equal(0, err, "%s: Unexcepted error", dev_name);

	err = counter_start(dev);
	zassert_equal(0, err, "%s: Unexcepted error", dev_name);

	k_busy_wait(2*tick_us);

	alarm_cfg.ticks = 0;
	err = counter_set_channel_alarm(dev, 0, &alarm_cfg);
	zassert_equal(-ETIME, err, "%s: Unexpected error (%d)", dev_name, err);

	/* wait couple of ticks */
	k_busy_wait(5*tick_us);

	tmp_alarm_cnt = alarm_cnt; /* to avoid passing volatile to the macro */
	zassert_equal(1, tmp_alarm_cnt,
			"%s: Expected %d callbacks, got %d\n",
			dev_name, 1, tmp_alarm_cnt);

	alarm_cfg.ticks = counter_read(dev);
	err = counter_set_channel_alarm(dev, 0, &alarm_cfg);
	zassert_equal(-ETIME, err, "%s: Failed to set an alarm (err: %d)",
			dev_name, err);

	/* wait to ensure that tick+1 timeout will expire. */
	k_busy_wait(3*tick_us);

	tmp_alarm_cnt = alarm_cnt; /* to avoid passing volatile to the macro */
	zassert_equal(2, tmp_alarm_cnt,
			"%s: Expected %d callbacks, got %d\n",
			dev_name, 2, tmp_alarm_cnt);
}

void test_late_alarm_error_instance(const char *dev_name)
{
	int err;
	struct device *dev = device_get_binding(dev_name);
	u32_t tick_us = (u32_t)counter_ticks_to_us(dev, 1);
	u32_t guard = counter_us_to_ticks(dev, 200);
	struct counter_alarm_cfg alarm_cfg = {
		.callback = alarm_handler,
		.flags = COUNTER_ALARM_CFG_ABSOLUTE,
		.user_data = NULL
	};

	err = counter_set_guard_period(dev, guard,
					COUNTER_GUARD_PERIOD_LATE_TO_SET);
	zassert_equal(0, err, "%s: Unexcepted error", dev_name);

	err = counter_start(dev);
	zassert_equal(0, err, "%s: Unexcepted error", dev_name);

	k_busy_wait(2*tick_us);

	alarm_cfg.ticks = 0;
	err = counter_set_channel_alarm(dev, 0, &alarm_cfg);
	zassert_equal(-ETIME, err,
			"%s: Failed to detect late setting (err: %d)",
			dev_name, err);

	alarm_cfg.ticks = counter_read(dev);
	err = counter_set_channel_alarm(dev, 0, &alarm_cfg);
	zassert_equal(-ETIME, err,
			"%s: Counter failed to detect late setting (err: %d)",
			dev_name, err);
}

static bool late_detection_capable(const char *dev_name)
{
	struct device *dev = device_get_binding(dev_name);
	u32_t guard = counter_get_guard_period(dev,
					COUNTER_GUARD_PERIOD_LATE_TO_SET);
	int err = counter_set_guard_period(dev, guard,
					COUNTER_GUARD_PERIOD_LATE_TO_SET);

	if (err == -ENOTSUP) {
		return false;
	}

	return true;
}

void test_late_alarm(void)
{
	test_all_instances(test_late_alarm_instance, late_detection_capable);
}

void test_late_alarm_error(void)
{
	test_all_instances(test_late_alarm_error_instance,
			   late_detection_capable);
}

static void test_short_relative_alarm_instance(const char *dev_name)
{
	int err;
	u32_t tmp_alarm_cnt;
	struct device *dev = device_get_binding(dev_name);
	u32_t tick_us = (u32_t)counter_ticks_to_us(dev, 1);
	struct counter_alarm_cfg alarm_cfg = {
		.callback = alarm_handler,
		.flags = 0,
		.user_data = NULL
	};

	err = counter_start(dev);
	zassert_equal(0, err, "%s: Unexcepted error", dev_name);

	alarm_cfg.ticks = 1;

	for (int i = 0; i < 100; ++i) {
		err = counter_set_channel_alarm(dev, 0, &alarm_cfg);
		zassert_equal(0, err,
				"%s: Failed to set an alarm (err: %d)",
				dev_name, err);

		/* wait to ensure that tick+1 timeout will expire. */
		k_busy_wait(3*tick_us);

		/* to avoid passing volatile to the macro */
		tmp_alarm_cnt = alarm_cnt;
		zassert_equal(i + 1, tmp_alarm_cnt,
				"%s: Expected %d callbacks, got %d\n",
				dev_name, i + 1, tmp_alarm_cnt);
	}
}

/* Function checks if relative alarm set for 1 tick will expire. If handler is
 * not called within near future it indicates that driver do not support it and
 * more extensive testing is skipped.
 */
static bool short_relative_capable(const char *dev_name)
{
	struct device *dev = device_get_binding(dev_name);
	struct counter_alarm_cfg alarm_cfg = {
		.callback = alarm_handler,
		.flags = 0,
		.user_data = NULL,
		.ticks = 1
	};
	int err;
	bool ret;

	if (single_channel_alarm_capable(dev_name) == false) {
		return false;
	}

	err = counter_start(dev);
	if (err != 0) {
		ret = false;
		goto end;
	}

	alarm_cnt = 0;
	err = counter_set_channel_alarm(dev, 0, &alarm_cfg);
	if (err != 0) {
		ret = false;
		goto end;
	}

	k_busy_wait(counter_ticks_to_us(dev, 10));
	if (alarm_cnt == 1) {
		ret = true;
	} else {
		ret = false;
		(void)counter_cancel_channel_alarm(dev, 0);
	}

end:
	alarm_cnt = 0;
	counter_stop(dev);
	k_busy_wait(1000);

	return ret;
}

static void test_short_relative_alarm(void)
{
	test_all_instances(test_short_relative_alarm_instance,
			short_relative_capable);
}

/* Test checks if cancelled alarm does not get triggered when new alarm is
 * configured at the point where previous alarm was about to expire.
 */
static void test_cancelled_alarm_does_not_expire_instance(const char *dev_name)
{
	int err;
	u32_t tmp_alarm_cnt;
	struct device *dev = device_get_binding(dev_name);
	u32_t us = 1000;
	u32_t ticks = counter_us_to_ticks(dev, us);

	us = (u32_t)counter_ticks_to_us(dev, ticks);

	struct counter_alarm_cfg alarm_cfg = {
		.callback = alarm_handler,
		.flags = COUNTER_ALARM_CFG_ABSOLUTE,
		.user_data = NULL
	};

	err = counter_start(dev);
	zassert_equal(0, err, "%s: Unexcepted error", dev_name);


	for (int i = 0; i < us/2; ++i) {
		alarm_cfg.ticks = counter_read(dev) + ticks;
		err = counter_set_channel_alarm(dev, 0, &alarm_cfg);
		zassert_equal(0, err, "%s: Failed to set an alarm (err: %d)",
				dev_name, err);

		err = counter_cancel_channel_alarm(dev, 0);
		zassert_equal(0, err, "%s: Failed to cancel an alarm (err: %d)",
				dev_name, err);

		k_busy_wait(us/2 + i);

		alarm_cfg.ticks = alarm_cfg.ticks + 2*alarm_cfg.ticks;
		err = counter_set_channel_alarm(dev, 0, &alarm_cfg);
		zassert_equal(0, err, "%s: Failed to set an alarm (err: %d)",
				dev_name, err);

		/* wait to ensure that tick+1 timeout will expire. */
		k_busy_wait(us);

		err = counter_cancel_channel_alarm(dev, 0);
		zassert_equal(0, err, "%s: Failed to cancel an alarm (err: %d)",
					dev_name, err);

		/* to avoid passing volatile to the macro */
		tmp_alarm_cnt = alarm_cnt;
		zassert_equal(0, tmp_alarm_cnt,
				"%s: Expected %d callbacks, got %d\n",
				dev_name, 0, tmp_alarm_cnt);
	}
}

static bool reliable_cancel_capable(const char *dev_name)
{
	/* Test performed only for NRF_RTC instances. Other probably will fail.
	 */
#ifdef CONFIG_COUNTER_RTC0
	/* Nordic RTC0 may be reserved for Bluetooth */
	if (strcmp(dev_name, DT_NORDIC_NRF_RTC_RTC_0_LABEL) == 0) {
		return true;
	}
#endif

#ifdef CONFIG_COUNTER_RTC2
	if (strcmp(dev_name, DT_NORDIC_NRF_RTC_RTC_2_LABEL) == 0) {
		return true;
	}
#endif

#ifdef CONFIG_COUNTER_TIMER0
	if (strcmp(dev_name, DT_NORDIC_NRF_TIMER_TIMER_0_LABEL) == 0) {
		return true;
	}
#endif

#ifdef CONFIG_COUNTER_TIMER1
	if (strcmp(dev_name, DT_NORDIC_NRF_TIMER_TIMER_1_LABEL) == 0) {
		return true;
	}
#endif

#ifdef CONFIG_COUNTER_TIMER2
	if (strcmp(dev_name, DT_NORDIC_NRF_TIMER_TIMER_2_LABEL) == 0) {
		return true;
	}
#endif

#ifdef CONFIG_COUNTER_TIMER3
	if (strcmp(dev_name, DT_NORDIC_NRF_TIMER_TIMER_3_LABEL) == 0) {
		return true;
	}
#endif

#ifdef CONFIG_COUNTER_TIMER4
	if (strcmp(dev_name, DT_NORDIC_NRF_TIMER_TIMER_4_LABEL) == 0) {
		return true;
	}
#endif
	return false;
}

void test_cancelled_alarm_does_not_expire(void)
{
	test_all_instances(test_cancelled_alarm_does_not_expire_instance,
			reliable_cancel_capable);
}

void test_main(void)
{
	ztest_test_suite(test_counter,
		ztest_unit_test(test_set_top_value_with_alarm),
		ztest_unit_test(test_single_shot_alarm_notop),
		ztest_unit_test(test_single_shot_alarm_top),
		ztest_unit_test(test_multiple_alarms),
		ztest_unit_test(test_all_channels),
		ztest_unit_test(test_late_alarm),
		ztest_unit_test(test_late_alarm_error),
		ztest_unit_test(test_short_relative_alarm),
		ztest_unit_test(test_cancelled_alarm_does_not_expire)
			 );
	ztest_run_test_suite(test_counter);
}
