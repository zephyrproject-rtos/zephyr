/*
 * Copyright (c) 2018, Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <counter.h>
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
#ifdef DT_ARM_CMSDK_TIMER_0_LABEL
	DT_ARM_CMSDK_TIMER_0_LABEL,
#endif
#ifdef DT_ARM_CMSDK_TIMER_1_LABEL
	DT_ARM_CMSDK_TIMER_1_LABEL,
#endif
#ifdef DT_ARM_CMSDK_DTIMER_0_LABEL
	DT_ARM_CMSDK_DTIMER_0_LABEL,
#endif
#ifdef DT_RTC_0_NAME
	DT_RTC_0_NAME,
#endif

#ifdef CONFIG_COUNTER_0_NAME
	CONFIG_COUNTER_0_NAME,
#endif
};
typedef void (*counter_test_func_t)(const char *dev_name);


static void counter_setup_instance(const char *dev_name)
{
	alarm_cnt = 0U;
}

static void counter_tear_down_instance(const char *dev_name)
{
	int err;
	struct device *dev;

	dev = device_get_binding(dev_name);

	err = counter_set_top_value(dev, counter_get_max_top_value(dev),
				    NULL, NULL);
	zassert_equal(0, err, "Setting top value to default failed\n");

	err = counter_stop(dev);
	zassert_equal(0, err, "Counter failed to stop\n");

}

static void test_all_instances(counter_test_func_t func)
{
	for (int i = 0; i < ARRAY_SIZE(devices); i++) {
		counter_setup_instance(devices[i]);
		func(devices[i]);
		counter_tear_down_instance(devices[i]);
	}
}

static void top_handler(struct device *dev, void *user_data)
{
	zassert_true(user_data == exp_user_data, "Unexpected callback\n");
	top_cnt++;
}

void test_set_top_value_with_alarm_instance(const char *dev_name)
{
	struct device *dev;
	int err;
	u32_t cnt;
	u32_t ticks;
	u32_t tmp_top_cnt;

	top_cnt = 0U;

	dev = device_get_binding(dev_name);
	ticks = counter_us_to_ticks(dev, COUNTER_PERIOD_US);

	err = counter_start(dev);
	zassert_equal(0, err, "Counter failed to start\n");

	k_busy_wait(5000);

	cnt = counter_read(dev);
	zassert_true(cnt > 0, "Counter should progress (dev:%s)\n", dev_name);

	err = counter_set_top_value(dev, ticks, top_handler, exp_user_data);
	zassert_equal(0, err, "Counter failed to set top value (dev:%s)\n",
			dev_name);

	k_busy_wait(5.2*COUNTER_PERIOD_US);

	tmp_top_cnt = top_cnt; /* to avoid passing volatile to the macro */
	zassert_true(tmp_top_cnt == 5U,
			"Unexpected number of turnarounds (%d) (dev: %s).\n",
			tmp_top_cnt, dev_name);
}

void test_set_top_value_with_alarm(void)
{
#if defined(CONFIG_COUNTER_MCUX_RTC) || defined(CONFIG_COUNTER_RTC_STM32)\
|| defined(DT_RTC_0_NAME)
	ztest_test_skip();
#else
	test_all_instances(test_set_top_value_with_alarm_instance);
#endif
}

static void alarm_handler(struct device *dev, u8_t chan_id, u32_t counter,
			  void *user_data)
{
	zassert_true(&alarm_cfg == user_data, "Unexpected callback\n");
	alarm_cnt++;
}

void test_single_shot_alarm_instance(const char *dev_name, bool set_top)
{
	struct device *dev;
	int err;
	u32_t ticks;
	u32_t tmp_alarm_cnt;

	dev = device_get_binding(dev_name);
	ticks = counter_us_to_ticks(dev, COUNTER_PERIOD_US);

	alarm_cfg.absolute = false;
	alarm_cfg.ticks = ticks;
	alarm_cfg.callback = alarm_handler;
	alarm_cfg.user_data = &alarm_cfg;

	alarm_cnt = 0U;

	if (counter_get_num_of_channels(dev) < 1U) {
		/* Counter does not support any alarm */
		return;
	}

	err = counter_start(dev);
	zassert_equal(0, err, "Counter failed to start\n");

	if (set_top) {
		err = counter_set_top_value(dev, ticks, top_handler,
					    exp_user_data);

		zassert_equal(0, err, "Counter failed to set top value\n");

		alarm_cfg.ticks = ticks + 1;
		err = counter_set_channel_alarm(dev, 0, &alarm_cfg);
		zassert_equal(-EINVAL, err,
			      "Counter should return error because ticks"
			      " exceeded the limit set alarm\n");
		alarm_cfg.ticks = ticks - 1;
	}

	err = counter_set_channel_alarm(dev, 0, &alarm_cfg);
	zassert_equal(0, err, "Counter set alarm failed\n");

	k_busy_wait(1.5*counter_ticks_to_us(dev, ticks));
	tmp_alarm_cnt = alarm_cnt; /* to avoid passing volatile to the macro */
	zassert_equal(1, tmp_alarm_cnt, "Expecting alarm callback\n");

	k_busy_wait(1.5*counter_ticks_to_us(dev, ticks));
	tmp_alarm_cnt = alarm_cnt; /* to avoid passing volatile to the macro */
	zassert_equal(1, tmp_alarm_cnt, "Expecting alarm callback\n");

	err = counter_cancel_channel_alarm(dev, 0);
	zassert_equal(0, err, "Counter disabling alarm failed\n");

	err = counter_set_top_value(dev, counter_get_max_top_value(dev),
				    NULL, NULL);
	zassert_equal(0, err, "Setting top value to default failed\n");

	err = counter_stop(dev);
	zassert_equal(0, err, "Counter failed to stop\n");
}

void test_single_shot_alarm_notop_instance(const char *dev_name)
{
	test_single_shot_alarm_instance(dev_name, false);
}

void test_single_shot_alarm_top_instance(const char *dev_name)
{
	test_single_shot_alarm_instance(dev_name, true);
}

void test_single_shot_alarm_notop(void)
{
	test_all_instances(test_single_shot_alarm_notop_instance);
}

void test_single_shot_alarm_top(void)
{
#if defined(CONFIG_COUNTER_MCUX_RTC) || defined(CONFIG_COUNTER_RTC_STM32)\
|| defined(DT_RTC_0_NAME)
	ztest_test_skip();
#else
	test_all_instances(test_single_shot_alarm_top_instance);
#endif
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

	dev = device_get_binding(dev_name);
	ticks = counter_us_to_ticks(dev, COUNTER_PERIOD_US);

	alarm_cfg.absolute = true;
	alarm_cfg.ticks = counter_us_to_ticks(dev, 2000);
	alarm_cfg.callback = alarm_handler2;
	alarm_cfg.user_data = &alarm_cfg;

	alarm_cfg2.absolute = false;
	alarm_cfg2.ticks = counter_us_to_ticks(dev, 2000);
	alarm_cfg2.callback = alarm_handler2;
	alarm_cfg2.user_data = &alarm_cfg2;

	alarm_cnt = 0U;

	if (counter_get_num_of_channels(dev) < 2U) {
		/* Counter does not support two alarms */
		return;
	}

	err = counter_start(dev);
	zassert_equal(0, err, "Counter failed to start\n");

	err = counter_set_top_value(dev, ticks, top_handler, exp_user_data);
	zassert_equal(0, err, "Counter failed to set top value\n");

	k_busy_wait(1.4*counter_ticks_to_us(dev, alarm_cfg.ticks));

	err = counter_set_channel_alarm(dev, 0, &alarm_cfg);
	zassert_equal(0, err, "Counter set alarm failed\n");

	err = counter_set_channel_alarm(dev, 1, &alarm_cfg2);
	zassert_equal(0, err, "Counter set alarm failed\n");

	k_busy_wait(1.2*counter_ticks_to_us(dev, ticks * 2U));
	tmp_alarm_cnt = alarm_cnt; /* to avoid passing volatile to the macro */
	zassert_equal(2, tmp_alarm_cnt, "Counter set alarm failed\n");
	zassert_equal(&alarm_cfg2, clbk_data[0],
			"Expected different order or callbacks\n");
	zassert_equal(&alarm_cfg, clbk_data[1],
			"Expected different order or callbacks\n");

	/* tear down */
	err = counter_cancel_channel_alarm(dev, 0);
	zassert_equal(0, err, "Counter disabling alarm failed\n");

	err = counter_cancel_channel_alarm(dev, 1);
	zassert_equal(0, err, "Counter disabling alarm failed\n");
}

void test_multiple_alarms(void)
{
	test_all_instances(test_multiple_alarms_instance);
}

void test_all_channels_instance(const char *str)
{
	struct device *dev;
	int err;
	const int n = 10;
	int nchan = 0;
	bool limit_reached = false;
	struct counter_alarm_cfg alarm_cfgs;
	u32_t ticks;
	u32_t tmp_alarm_cnt;

	dev = device_get_binding(str);
	ticks = counter_us_to_ticks(dev, COUNTER_PERIOD_US);

	alarm_cfgs.absolute = false;
	alarm_cfgs.ticks = ticks;
	alarm_cfgs.callback = alarm_handler2;
	alarm_cfgs.user_data = NULL;

	err = counter_start(dev);
	zassert_equal(0, err, "Counter failed to start");

	for (int i = 0; i < n; i++) {
		err = counter_set_channel_alarm(dev, i, &alarm_cfgs);
		if ((err == 0) && !limit_reached) {
			nchan++;
		} else if (err == -ENOTSUP) {
			limit_reached = true;
		} else {
			zassert_equal(0, 1,
					"Unexpected error on setting alarm");
		}
	}

	k_busy_wait(1.5*counter_ticks_to_us(dev, ticks));
	tmp_alarm_cnt = alarm_cnt; /* to avoid passing volatile to the macro */
	zassert_equal(nchan, tmp_alarm_cnt, "Expecting alarm callback\n");

	for (int i = 0; i < nchan; i++) {
		err = counter_cancel_channel_alarm(dev, i);
		zassert_equal(0, err, "Unexpected error on disabling alarm");
	}

	for (int i = nchan; i < n; i++) {
		err = counter_cancel_channel_alarm(dev, i);
		zassert_equal(-ENOTSUP, err,
				"Unexpected error on disabling alarm\n");
	}
}

void test_all_channels(void)
{
	test_all_instances(test_all_channels_instance);
}

void test_main(void)
{
	ztest_test_suite(test_counter,
		ztest_unit_test(test_set_top_value_with_alarm),
		ztest_unit_test(test_single_shot_alarm_notop),
		ztest_unit_test(test_single_shot_alarm_top),
		ztest_unit_test(test_multiple_alarms),
		ztest_unit_test(test_all_channels)
			 );
	ztest_run_test_suite(test_counter);
}
