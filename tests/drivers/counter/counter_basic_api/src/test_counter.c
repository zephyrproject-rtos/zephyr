/*
 * Copyright (c) 2018, Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <drivers/counter.h>
#include <ztest.h>
#include <kernel.h>

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

#ifdef DT_INST_0_ARM_CMSDK_TIMER_LABEL
#define DT_INST_0_ARM_CMSDK_TIMER_PRESENT 1
#endif

#ifdef DT_INST_1_ARM_CMSDK_TIMER_LABEL
#define DT_INST_1_ARM_CMSDK_TIMER_PRESENT 1
#endif

#ifdef DT_INST_0_ARM_CMSDK_DTIMER_LABEL
#define DT_INST_0_ARM_CMSDK_DTIMER_PRESENT 1
#endif

#ifdef DT_RTC_0_NAME
#define	DT_RTC_0_PRESENT 1
#endif

#ifdef CONFIG_COUNTER_0_NAME
#define CONFIG_COUNTER_0_PRESENT 1
#endif

#define DEVICES_LIST \
	COND_CODE_1(CONFIG_COUNTER_TIMER0, \
		    (DT_NORDIC_NRF_TIMER_TIMER_0_LABEL,), ()) \
	COND_CODE_1(CONFIG_COUNTER_TIMER1, \
		    (DT_NORDIC_NRF_TIMER_TIMER_1_LABEL,), ()) \
	COND_CODE_1(CONFIG_COUNTER_TIMER2, \
		    (DT_NORDIC_NRF_TIMER_TIMER_2_LABEL,), ()) \
	COND_CODE_1(CONFIG_COUNTER_TIMER3, \
		    (DT_NORDIC_NRF_TIMER_TIMER_3_LABEL,), ()) \
	COND_CODE_1(CONFIG_COUNTER_TIMER4, \
		    (DT_NORDIC_NRF_TIMER_TIMER_4_LABEL,), ()) \
	COND_CODE_1(CONFIG_COUNTER_RTC0, \
		    (DT_NORDIC_NRF_RTC_RTC_0_LABEL,), ()) \
	/* Nordic RTC1 is used for the system clock */ \
	COND_CODE_1(CONFIG_COUNTER_RTC2, \
		    (DT_NORDIC_NRF_RTC_RTC_2_LABEL,), ()) \
	COND_CODE_1(CONFIG_COUNTER_IMX_EPIT_1, \
		    (DT_COUNTER_IMX_EPIT_1_LABEL,), ()) \
	COND_CODE_1(CONFIG_COUNTER_IMX_EPIT_2, \
		    (DT_COUNTER_IMX_EPIT_2_LABEL,), ()) \
	COND_CODE_1(CONFIG_COUNTER_MCUX_RTC, \
		    (DT_RTC_MCUX_0_NAME,), ()) \
	COND_CODE_1(CONFIG_COUNTER_MCUX_RTC, \
		    (DT_INST_0_ARM_CMSDK_TIMER_LABEL,), ()) \
	COND_CODE_1(DT_INST_0_ARM_CMSDK_TIMER_PRESENT, \
		    (DT_INST_0_ARM_CMSDK_TIMER_LABEL,), ()) \
	COND_CODE_1(DT_INST_1_ARM_CMSDK_TIMER_PRESENT, \
		    (DT_INST_1_ARM_CMSDK_TIMER_LABEL,), ()) \
	COND_CODE_1(DT_INST_0_ARM_CMSDK_DTIMER_PRESENT, \
		    (DT_INST_0_ARM_CMSDK_DTIMER_LABEL,), ()) \
	COND_CODE_1(DT_RTC_0_PRESENT, \
		    (DT_RTC_0_NAME,), ()) \
	COND_CODE_1(CONFIG_COUNTER_0_PRESENT, \
		    (CONFIG_COUNTER_0_NAME,), ())

const char *devices[] = {
	DEVICES_LIST
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

/* Function perfoms given test on one device instance. Optionally, it can
 * check if instance is capable to perform the test. If not test is skipped
 * for given instance.
 */
static void test_instance(u32_t dev_idx, counter_test_func_t test_func,
			  counter_capability_func_t capability_check)
{
	const char *name = devices[dev_idx];

	TC_PRINT("Testing %s device\n", name);

	counter_setup_instance(name);

	if ((capability_check == NULL) ||
	     capability_check(name)) {
		test_func(name);
	} else {
		ztest_test_skip();
	}

	counter_tear_down_instance(name);

	/* Allow logs to be printed. */
	k_sleep(100);
}

static void top_handler(struct device *dev, void *user_data)
{
	zassert_true(user_data == exp_user_data,
			"%s: Unexpected callback", dev->config->name);
	top_cnt++;
}

static void change_top_value_on_instance(const char *dev_name)
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
	zassert_equal(0, err, "%s: Counter failed to set top value",
			dev_name);

	k_busy_wait(5.2*COUNTER_PERIOD_US);

	tmp_top_cnt = top_cnt; /* to avoid passing volatile to the macro */
	zassert_true(tmp_top_cnt == 5U,
			"%s: Unexpected number of turnarounds (%d).",
			dev_name, tmp_top_cnt);
}

static bool custom_top_value_capable(const char *dev_name)
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

static void change_top_value(u32_t dev_idx)
{
	test_instance(dev_idx, change_top_value_on_instance,
			custom_top_value_capable);
}

ztest_unit_tests_from_list_create(change_top_value, DEVICES_LIST)

static void alarm_handler(struct device *dev, u8_t chan_id, u32_t counter,
			  void *user_data)
{
	u32_t now = counter_read(dev);

	zassert_true(&alarm_cfg == user_data,
			"%s: Unexpected callback", dev->config->name);
	zassert_true(now >= counter,
			"%s: Alarm (%d) too early now:%d.",
			dev->config->name, counter);
	alarm_cnt++;
}

static void test_single_shot_alarm_instance(const char *dev_name, bool set_top)
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
	alarm_cfg.ticks = ticks;
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

	err = counter_set_channel_alarm(dev, 0, &alarm_cfg);
	zassert_equal(0, err, "%s: Counter set alarm failed", dev_name);

	k_busy_wait(1.5*counter_ticks_to_us(dev, ticks));
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

static bool single_alarm_capable(const char *dev_name)
{
	struct device *dev = device_get_binding(dev_name);
	const struct counter_config_info *config =
			(struct counter_config_info *)dev->config->config_info;

	return config->channels > 0;
}

static bool single_alarm_and_top_value_capable(const char *dev_name)
{
	return single_alarm_capable(dev_name) &&
		custom_top_value_capable(dev_name);
}

static void test_single_shot_alarm_notop_on_instance(const char *dev_name)
{
	test_single_shot_alarm_instance(dev_name, false);
}

static void test_single_shot_alarm_top_on_instance(const char *dev_name)
{
	test_single_shot_alarm_instance(dev_name, true);
}

static void single_shot_alarm_notop(u32_t dev_idx)
{
	test_instance(dev_idx, test_single_shot_alarm_notop_on_instance,
			single_alarm_capable);
}

ztest_unit_tests_from_list_create(single_shot_alarm_notop, DEVICES_LIST)

static void single_shot_alarm_top(u32_t dev_idx)
{
	test_instance(dev_idx, test_single_shot_alarm_top_on_instance,
			single_alarm_and_top_value_capable);
}

ztest_unit_tests_from_list_create(single_shot_alarm_top, DEVICES_LIST)


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
static void test_multiple_alarms_instance(const char *dev_name)
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

	k_busy_wait(1.4*counter_ticks_to_us(dev, alarm_cfg.ticks));

	err = counter_set_channel_alarm(dev, 0, &alarm_cfg);
	zassert_equal(0, err, "%s: Counter set alarm failed", dev_name);

	err = counter_set_channel_alarm(dev, 1, &alarm_cfg2);
	zassert_equal(0, err, "%s: Counter set alarm failed", dev_name);

	k_busy_wait(1.2*counter_ticks_to_us(dev, ticks * 2U));
	tmp_alarm_cnt = alarm_cnt; /* to avoid passing volatile to the macro */
	zassert_equal(2, tmp_alarm_cnt, "%s: Counter set alarm failed",
			dev_name);
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

static bool multi_alarm_capable(const char *dev_name)
{
	struct device *dev = device_get_binding(dev_name);
	const struct counter_config_info *config =
			(struct counter_config_info *)dev->config->config_info;

	return config->channels > 1;
}

static void multiple_alarms(u32_t dev_idx)
{
	test_instance(dev_idx, test_multiple_alarms_instance,
			multi_alarm_capable);
}

ztest_unit_tests_from_list_create(multiple_alarms, DEVICES_LIST)

static void test_all_channels_instance(const char *dev_name)
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

/* Test alarm on all channels */
static void all_channels(u32_t dev_idx)
{
	test_instance(dev_idx, test_all_channels_instance,
			multi_alarm_capable);
}

ztest_unit_tests_from_list_create(all_channels, DEVICES_LIST)

void test_main(void)
{
	ztest_test_suite(test_counter,
	  ztest_unit_tests_from_list(change_top_value, DEVICES_LIST),
	  ztest_unit_tests_from_list(single_shot_alarm_notop, DEVICES_LIST),
	  ztest_unit_tests_from_list(single_shot_alarm_top, DEVICES_LIST),
	  ztest_unit_tests_from_list(multiple_alarms, DEVICES_LIST),
	  ztest_unit_tests_from_list(all_channels, DEVICES_LIST)
	);
	ztest_run_test_suite(test_counter);
}
