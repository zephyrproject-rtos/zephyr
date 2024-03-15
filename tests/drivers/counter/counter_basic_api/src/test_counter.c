/*
 * Copyright (c) 2018, Nordic Semiconductor ASA
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/counter.h>
#include <zephyr/ztest.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(test);

static struct k_sem top_cnt_sem;
static volatile uint32_t top_cnt;
static struct k_sem alarm_cnt_sem;
static volatile uint32_t alarm_cnt;

static void top_handler(const struct device *dev, void *user_data);

void *exp_user_data = (void *)199;

struct counter_alarm_cfg cntr_alarm_cfg;
struct counter_alarm_cfg cntr_alarm_cfg2;

#define DEVICE_DT_GET_AND_COMMA(node_id) DEVICE_DT_GET(node_id),
/* Generate a list of devices for all instances of the "compat" */
#define DEVS_FOR_DT_COMPAT(compat) \
	DT_FOREACH_STATUS_OKAY(compat, DEVICE_DT_GET_AND_COMMA)

static const struct device *const devices[] = {
#ifdef CONFIG_COUNTER_NRF_TIMER
	DEVS_FOR_DT_COMPAT(nordic_nrf_timer)
#endif
#ifdef CONFIG_COUNTER_NRF_RTC
	DEVS_FOR_DT_COMPAT(nordic_nrf_rtc)
#endif
#ifdef CONFIG_COUNTER_TIMER_STM32
#define STM32_COUNTER_DEV(idx) \
	DEVICE_DT_GET(DT_INST(idx, st_stm32_counter)),
#define DT_DRV_COMPAT st_stm32_counter
	DT_INST_FOREACH_STATUS_OKAY(STM32_COUNTER_DEV)
#undef DT_DRV_COMPAT
#undef STM32_COUNTER_DEV
#endif
#ifdef CONFIG_COUNTER_NATIVE_POSIX
	DEVICE_DT_GET(DT_NODELABEL(counter0)),
#endif
#ifdef CONFIG_COUNTER_INFINEON_CAT1
	DEVICE_DT_GET(DT_NODELABEL(counter0_0)),
#endif
	/* NOTE: there is no trailing comma, as the DEVS_FOR_DT_COMPAT
	 * handles it.
	 */
	DEVS_FOR_DT_COMPAT(arm_cmsdk_timer)
	DEVS_FOR_DT_COMPAT(arm_cmsdk_dtimer)
	DEVS_FOR_DT_COMPAT(microchip_xec_timer)
	DEVS_FOR_DT_COMPAT(nxp_imx_epit)
	DEVS_FOR_DT_COMPAT(nxp_imx_gpt)
	DEVS_FOR_DT_COMPAT(renesas_smartbond_timer)
#ifdef CONFIG_COUNTER_MCUX_CTIMER
	DEVS_FOR_DT_COMPAT(nxp_lpc_ctimer)
#endif
#ifdef CONFIG_COUNTER_MCUX_RTC
	DEVS_FOR_DT_COMPAT(nxp_kinetis_rtc)
#endif
#ifdef CONFIG_COUNTER_MCUX_QTMR
	DEVS_FOR_DT_COMPAT(nxp_imx_tmr)
#endif
#ifdef CONFIG_COUNTER_NXP_MRT
	DEVS_FOR_DT_COMPAT(nxp_mrt_channel)
#endif
#ifdef CONFIG_COUNTER_MCUX_LPC_RTC_1HZ
	DEVS_FOR_DT_COMPAT(nxp_lpc_rtc)
#endif
#ifdef CONFIG_COUNTER_MCUX_LPC_RTC_HIGHRES
	DEVS_FOR_DT_COMPAT(nxp_lpc_rtc_highres)
#endif
#ifdef CONFIG_COUNTER_GECKO_RTCC
	DEVS_FOR_DT_COMPAT(silabs_gecko_rtcc)
#endif
#ifdef CONFIG_COUNTER_RTC_STM32
	DEVS_FOR_DT_COMPAT(st_stm32_rtc)
#endif
#ifdef CONFIG_COUNTER_GECKO_STIMER
	DEVS_FOR_DT_COMPAT(silabs_gecko_stimer)
#endif
#ifdef CONFIG_COUNTER_NXP_PIT
	DEVS_FOR_DT_COMPAT(nxp_pit_channel)
#endif
#ifdef CONFIG_COUNTER_XLNX_AXI_TIMER
	DEVS_FOR_DT_COMPAT(xlnx_xps_timer_1_00_a)
#endif
#ifdef CONFIG_COUNTER_TMR_ESP32
	DEVS_FOR_DT_COMPAT(espressif_esp32_timer)
#endif
#ifdef CONFIG_COUNTER_NXP_S32_SYS_TIMER
	DEVS_FOR_DT_COMPAT(nxp_s32_sys_timer)
#endif
#ifdef CONFIG_COUNTER_TIMER_GD32
	DEVS_FOR_DT_COMPAT(gd_gd32_timer)
#endif
#ifdef CONFIG_COUNTER_TIMER_RPI_PICO
	DEVS_FOR_DT_COMPAT(raspberrypi_pico_timer)
#endif
#ifdef CONFIG_COUNTER_AMBIQ
	DEVS_FOR_DT_COMPAT(ambiq_counter)
#endif
};

static const struct device *const period_devs[] = {
#ifdef CONFIG_COUNTER_MCUX_RTC
	DEVS_FOR_DT_COMPAT(nxp_kinetis_rtc)
#endif
#ifdef CONFIG_COUNTER_MCUX_LPC_RTC
	DEVS_FOR_DT_COMPAT(nxp_lpc_rtc)
#endif
	DEVS_FOR_DT_COMPAT(st_stm32_rtc)
};

typedef void (*counter_test_func_t)(const struct device *dev);

typedef bool (*counter_capability_func_t)(const struct device *dev);

static inline uint32_t get_counter_period_us(const struct device *dev)
{
	for (int i = 0; i < ARRAY_SIZE(period_devs); i++) {
		if (period_devs[i] == dev) {
			return (USEC_PER_SEC * 2U);
		}
	}

	/* if more counter drivers exist other than RTC,
	 * the test value set to 20000 by default
	 */
	return 20000;
}

static void counter_setup_instance(const struct device *dev)
{
	k_sem_reset(&alarm_cnt_sem);
	if (!k_is_user_context()) {
		alarm_cnt = 0;
	}
}

static void counter_tear_down_instance(const struct device *dev)
{
	int err;
	struct counter_top_cfg top_cfg = {
		.callback = NULL,
		.user_data = NULL,
		.flags = 0
	};

	top_cfg.ticks = counter_get_max_top_value(dev);
	err = counter_set_top_value(dev, &top_cfg);
	if (err == -ENOTSUP) {
		/* If resetting is not support, attempt without reset. */
		top_cfg.flags = COUNTER_TOP_CFG_DONT_RESET;
		err = counter_set_top_value(dev, &top_cfg);

	}
	zassert_true((err == 0) || (err == -ENOTSUP),
			"%s: Setting top value to default failed", dev->name);

	err = counter_stop(dev);
	zassert_equal(0, err, "%s: Counter failed to stop", dev->name);

}

static void test_all_instances(counter_test_func_t func,
				counter_capability_func_t capability_check)
{
	zassert_true(ARRAY_SIZE(devices) > 0, "No device found");
	for (int i = 0; i < ARRAY_SIZE(devices); i++) {
		counter_setup_instance(devices[i]);
		if ((capability_check == NULL) ||
		     capability_check(devices[i])) {
			TC_PRINT("Testing %s\n", devices[i]->name);
			func(devices[i]);
		} else {
			TC_PRINT("Skipped for %s\n", devices[i]->name);
		}
		counter_tear_down_instance(devices[i]);
		/* Allow logs to be printed. */
		k_sleep(K_MSEC(100));
	}
}

static bool set_top_value_capable(const struct device *dev)
{
	struct counter_top_cfg cfg = {
		.ticks = counter_get_top_value(dev) - 1
	};
	int err;

	err = counter_set_top_value(dev, &cfg);
	if (err == -ENOTSUP) {
		return false;
	}

	cfg.ticks++;
	err = counter_set_top_value(dev, &cfg);
	if (err == -ENOTSUP) {
		return false;
	}

	return true;
}

static void top_handler(const struct device *dev, void *user_data)
{
	zassert_true(user_data == exp_user_data,
			"%s: Unexpected callback", dev->name);
	if (IS_ENABLED(CONFIG_ZERO_LATENCY_IRQS)) {
		top_cnt++;

		return;
	}

	k_sem_give(&top_cnt_sem);
}

static void test_set_top_value_with_alarm_instance(const struct device *dev)
{
	int err;
	uint32_t cnt;
	uint32_t top_value;
	uint32_t counter_period_us;
	uint32_t top_handler_cnt;
	struct counter_top_cfg top_cfg = {
		.callback = top_handler,
		.user_data = exp_user_data,
		.flags = 0
	};

	k_sem_reset(&top_cnt_sem);
	top_cnt = 0;

	counter_period_us = get_counter_period_us(dev);
	top_cfg.ticks = counter_us_to_ticks(dev, counter_period_us);
	err = counter_start(dev);
	zassert_equal(0, err, "%s: Counter failed to start", dev->name);

	k_busy_wait(5000);

	err = counter_get_value(dev, &cnt);
	zassert_true(err == 0, "%s: Counter read failed (err: %d)", dev->name,
		     err);
	if (counter_is_counting_up(dev)) {
		err = (cnt > 0) ? 0 : 1;
	} else {
		top_value = counter_get_top_value(dev);
		err = (cnt < top_value) ? 0 : 1;
	}
	zassert_true(err == 0, "%s: Counter should progress", dev->name);

	err = counter_set_top_value(dev, &top_cfg);
	zassert_equal(0, err, "%s: Counter failed to set top value (err: %d)",
			dev->name, err);

	k_busy_wait(5.2*counter_period_us);

	top_handler_cnt = IS_ENABLED(CONFIG_ZERO_LATENCY_IRQS) ?
		top_cnt : k_sem_count_get(&top_cnt_sem);
	zassert_true(top_handler_cnt == 5U,
			"%s: Unexpected number of turnarounds (%d).",
			dev->name, top_handler_cnt);
}

ZTEST(counter_basic, test_set_top_value_with_alarm)
{
	test_all_instances(test_set_top_value_with_alarm_instance,
			   set_top_value_capable);
}

static void test_set_top_value_without_alarm_instance(const struct device *dev)
{
	int err;
	uint32_t cnt;
	uint32_t top_value;
	uint32_t counter_period_us;
	struct counter_top_cfg top_cfg = {
		.callback = NULL,
		.user_data = NULL,
		.flags = 0
	};

	counter_period_us = get_counter_period_us(dev);
	top_cfg.ticks = counter_us_to_ticks(dev, counter_period_us);
	err = counter_start(dev);
	zassert_equal(0, err, "%s: Counter failed to start", dev->name);

	k_busy_wait(5000);

	err = counter_get_value(dev, &cnt);
	zassert_true(err == 0, "%s: Counter read failed (err: %d)", dev->name,
		     err);
	if (counter_is_counting_up(dev)) {
		err = (cnt > 0) ? 0 : 1;
	} else {
		top_value = counter_get_top_value(dev);
		err = (cnt < top_value) ? 0 : 1;
	}
	zassert_true(err == 0, "%s: Counter should progress", dev->name);

	err = counter_set_top_value(dev, &top_cfg);
	zassert_equal(0, err, "%s: Counter failed to set top value (err: %d)",
			dev->name, err);

	zassert_true(counter_get_top_value(dev) == top_cfg.ticks,
			"%s: new top value not in use.",
			dev->name);
}

ZTEST_USER(counter_no_callback, test_set_top_value_without_alarm)
{
	test_all_instances(test_set_top_value_without_alarm_instance,
			   set_top_value_capable);
}

static void alarm_handler(const struct device *dev, uint8_t chan_id,
			  uint32_t counter,
			  void *user_data)
{
	/* Arbitrary limit for alarm processing - time between hw expiration
	 * and read-out from counter in the handler.
	 */
	static const uint64_t processing_limit_us = 1000;
	uint32_t now;
	int err;
	uint32_t top;
	uint32_t diff;

	err = counter_get_value(dev, &now);
	zassert_true(err == 0, "%s: Counter read failed (err: %d)",
		     dev->name, err);

	top = counter_get_top_value(dev);
	if (counter_is_counting_up(dev)) {
		diff =  (now < counter) ?
			(now + top - counter) : (now - counter);
	} else {
		diff = (now > counter) ?
			(counter + top - now) : (counter - now);
	}

	zassert_true(diff <= counter_us_to_ticks(dev, processing_limit_us),
			"Unexpected distance between reported alarm value(%u) "
			"and actual counter value (%u), top:%d (processing "
			"time limit (%d us) might be exceeded?",
			counter, now, top, processing_limit_us);

	if (user_data) {
		zassert_true(&cntr_alarm_cfg == user_data,
			"%s: Unexpected callback", dev->name);
	}

	if (IS_ENABLED(CONFIG_ZERO_LATENCY_IRQS)) {
		alarm_cnt++;
		return;
	}
	zassert_true(k_is_in_isr(), "%s: Expected interrupt context",
			dev->name);
	k_sem_give(&alarm_cnt_sem);
}

static void test_single_shot_alarm_instance(const struct device *dev, bool set_top)
{
	int err;
	uint32_t ticks;
	uint32_t cnt;
	uint32_t counter_period_us;
	struct counter_top_cfg top_cfg = {
		.callback = top_handler,
		.user_data = exp_user_data,
		.flags = 0
	};

	counter_period_us = get_counter_period_us(dev);
	ticks = counter_us_to_ticks(dev, counter_period_us);
	top_cfg.ticks = ticks;

	cntr_alarm_cfg.flags = 0;
	cntr_alarm_cfg.callback = alarm_handler;
	cntr_alarm_cfg.user_data = &cntr_alarm_cfg;

	k_sem_reset(&alarm_cnt_sem);
	alarm_cnt = 0;

	if (counter_get_num_of_channels(dev) < 1U) {
		/* Counter does not support any alarm */
		return;
	}

	err = counter_start(dev);
	zassert_equal(0, err, "%s: Counter failed to start", dev->name);

	if (set_top) {
		err = counter_set_top_value(dev, &top_cfg);

		zassert_equal(0, err,
			     "%s: Counter failed to set top value", dev->name);

		cntr_alarm_cfg.ticks = ticks + 1;
		err = counter_set_channel_alarm(dev, 0, &cntr_alarm_cfg);
		zassert_equal(-EINVAL, err,
			      "%s: Counter should return error because ticks"
			      " exceeded the limit set alarm", dev->name);
		cntr_alarm_cfg.ticks = ticks - 1;
	}

	cntr_alarm_cfg.ticks = ticks;
	err = counter_set_channel_alarm(dev, 0, &cntr_alarm_cfg);
	zassert_equal(0, err, "%s: Counter set alarm failed (err: %d)",
			dev->name, err);

	k_busy_wait(2*(uint32_t)counter_ticks_to_us(dev, ticks));

	cnt = IS_ENABLED(CONFIG_ZERO_LATENCY_IRQS) ?
		alarm_cnt : k_sem_count_get(&alarm_cnt_sem);
	zassert_equal(1, cnt, "%s: Expecting alarm callback", dev->name);

	k_busy_wait(1.5*counter_ticks_to_us(dev, ticks));
	cnt = IS_ENABLED(CONFIG_ZERO_LATENCY_IRQS) ?
		alarm_cnt : k_sem_count_get(&alarm_cnt_sem);
	zassert_equal(1, cnt, "%s: Expecting alarm callback", dev->name);

	err = counter_cancel_channel_alarm(dev, 0);
	zassert_equal(0, err, "%s: Counter disabling alarm failed", dev->name);

	top_cfg.ticks = counter_get_max_top_value(dev);
	top_cfg.callback = NULL;
	top_cfg.user_data = NULL;
	err = counter_set_top_value(dev, &top_cfg);
	if (err == -ENOTSUP) {
		/* If resetting is not support, attempt without reset. */
		top_cfg.flags = COUNTER_TOP_CFG_DONT_RESET;
		err = counter_set_top_value(dev, &top_cfg);

	}
	zassert_true((err == 0) || (err == -ENOTSUP),
			"%s: Setting top value to default failed", dev->name);

	err = counter_stop(dev);
	zassert_equal(0, err, "%s: Counter failed to stop", dev->name);
}

void test_single_shot_alarm_notop_instance(const struct device *dev)
{
	test_single_shot_alarm_instance(dev, false);
}

void test_single_shot_alarm_top_instance(const struct device *dev)
{
	test_single_shot_alarm_instance(dev, true);
}

static bool single_channel_alarm_capable(const struct device *dev)
{
	return (counter_get_num_of_channels(dev) > 0);
}

static bool single_channel_alarm_and_custom_top_capable(const struct device *dev)
{
	return single_channel_alarm_capable(dev) &&
		set_top_value_capable(dev);
}

ZTEST(counter_basic, test_single_shot_alarm_notop)
{
	test_all_instances(test_single_shot_alarm_notop_instance,
			   single_channel_alarm_capable);
}

ZTEST(counter_basic, test_single_shot_alarm_top)
{
	test_all_instances(test_single_shot_alarm_top_instance,
			   single_channel_alarm_and_custom_top_capable);
}

static void *clbk_data[10];

static void alarm_handler2(const struct device *dev, uint8_t chan_id,
			   uint32_t counter,
			   void *user_data)
{
	if (IS_ENABLED(CONFIG_ZERO_LATENCY_IRQS)) {
		clbk_data[alarm_cnt] = user_data;
		alarm_cnt++;

		return;
	}

	clbk_data[k_sem_count_get(&alarm_cnt_sem)] = user_data;
	k_sem_give(&alarm_cnt_sem);
}

/*
 * Two alarms set. First alarm is absolute, second relative. Because
 * setting of both alarms is delayed it is expected that second alarm
 * will expire first (relative to the time called) while first alarm
 * will expire after next wrap around.
 */
static void test_multiple_alarms_instance(const struct device *dev)
{
	int err;
	uint32_t ticks;
	uint32_t cnt;
	uint32_t counter_period_us;
	struct counter_top_cfg top_cfg = {
		.callback = top_handler,
		.user_data = exp_user_data,
		.flags = 0
	};

	counter_period_us = get_counter_period_us(dev);
	ticks = counter_us_to_ticks(dev, counter_period_us);

	err = counter_get_value(dev, &(top_cfg.ticks));
	zassert_equal(0, err, "%s: Counter get value failed", dev->name);
	top_cfg.ticks += ticks;

	cntr_alarm_cfg.flags = COUNTER_ALARM_CFG_ABSOLUTE;
	cntr_alarm_cfg.ticks = counter_us_to_ticks(dev, 2000);
	cntr_alarm_cfg.callback = alarm_handler2;
	cntr_alarm_cfg.user_data = &cntr_alarm_cfg;

	cntr_alarm_cfg2.flags = 0;
	cntr_alarm_cfg2.ticks = counter_us_to_ticks(dev, 2000);
	cntr_alarm_cfg2.callback = alarm_handler2;
	cntr_alarm_cfg2.user_data = &cntr_alarm_cfg2;

	k_sem_reset(&alarm_cnt_sem);
	alarm_cnt = 0;

	if (counter_get_num_of_channels(dev) < 2U) {
		/* Counter does not support two alarms */
		return;
	}

	err = counter_start(dev);
	zassert_equal(0, err, "%s: Counter failed to start", dev->name);

	if (set_top_value_capable(dev)) {
		err = counter_set_top_value(dev, &top_cfg);
		zassert_equal(0, err, "%s: Counter failed to set top value", dev->name);
	} else {
		/* Counter does not support top value, do not run this test
		 * as it might take a long time to wrap and trigger the alarm
		 * resulting in test failures.
		 */
		return;
	}

	k_busy_wait(3*(uint32_t)counter_ticks_to_us(dev, cntr_alarm_cfg.ticks));

	err = counter_set_channel_alarm(dev, 0, &cntr_alarm_cfg);
	zassert_equal(0, err, "%s: Counter set alarm failed", dev->name);

	err = counter_set_channel_alarm(dev, 1, &cntr_alarm_cfg2);
	zassert_equal(0, err, "%s: Counter set alarm failed", dev->name);

	k_busy_wait(1.2 * counter_ticks_to_us(dev, ticks * 2U));

	cnt = IS_ENABLED(CONFIG_ZERO_LATENCY_IRQS) ?
		alarm_cnt : k_sem_count_get(&alarm_cnt_sem);
	zassert_equal(2, cnt,
			"%s: Invalid number of callbacks %d (expected: %d)",
			dev->name, cnt, 2);

	zassert_equal(&cntr_alarm_cfg2, clbk_data[0],
			"%s: Expected different order or callbacks",
			dev->name);
	zassert_equal(&cntr_alarm_cfg, clbk_data[1],
			"%s: Expected different order or callbacks",
			dev->name);

	/* tear down */
	err = counter_cancel_channel_alarm(dev, 0);
	zassert_equal(0, err, "%s: Counter disabling alarm failed", dev->name);

	err = counter_cancel_channel_alarm(dev, 1);
	zassert_equal(0, err, "%s: Counter disabling alarm failed", dev->name);
}

static bool multiple_channel_alarm_capable(const struct device *dev)
{
	return (counter_get_num_of_channels(dev) > 1);
}

ZTEST(counter_basic, test_multiple_alarms)
{
	test_all_instances(test_multiple_alarms_instance,
			   multiple_channel_alarm_capable);
}

static void test_all_channels_instance(const struct device *dev)
{
	int err;
	const int n = 10;
	int nchan = 0;
	bool limit_reached = false;
	struct counter_alarm_cfg alarm_cfgs;
	uint32_t ticks;
	uint32_t cnt;
	uint32_t counter_period_us;

	counter_period_us = get_counter_period_us(dev);
	ticks = counter_us_to_ticks(dev, counter_period_us);

	alarm_cfgs.flags = 0;
	alarm_cfgs.ticks = ticks;
	alarm_cfgs.callback = alarm_handler2;
	alarm_cfgs.user_data = NULL;

	err = counter_start(dev);
	zassert_equal(0, err, "%s: Counter failed to start", dev->name);

	for (int i = 0; i < n; i++) {
		err = counter_set_channel_alarm(dev, i, &alarm_cfgs);
		if ((err == 0) && !limit_reached) {
			nchan++;
		} else if (err == -ENOTSUP) {
			limit_reached = true;
		} else {
			zassert_equal(0, 1,
			   "%s: Unexpected error on setting alarm", dev->name);
		}
	}

	k_busy_wait(1.5*counter_ticks_to_us(dev, ticks));
	cnt = IS_ENABLED(CONFIG_ZERO_LATENCY_IRQS) ?
		alarm_cnt : k_sem_count_get(&alarm_cnt_sem);
	zassert_equal(nchan, cnt,
			"%s: Expecting alarm callback", dev->name);

	for (int i = 0; i < nchan; i++) {
		err = counter_cancel_channel_alarm(dev, i);
		zassert_equal(0, err,
			"%s: Unexpected error on disabling alarm", dev->name);
	}

	for (int i = nchan; i < n; i++) {
		err = counter_cancel_channel_alarm(dev, i);
		zassert_equal(-ENOTSUP, err,
			"%s: Unexpected error on disabling alarm", dev->name);
	}
}

ZTEST(counter_basic, test_all_channels)
{
	test_all_instances(test_all_channels_instance,
			   single_channel_alarm_capable);
}

/**
 * Test validates if alarm set too late (current tick or current tick + 1)
 * results in callback being called.
 */
static void test_late_alarm_instance(const struct device *dev)
{
	int err;
	uint32_t cnt;
	uint32_t tick_us = (uint32_t)counter_ticks_to_us(dev, 1);
	uint32_t guard = counter_us_to_ticks(dev, 200);
	struct counter_alarm_cfg alarm_cfg = {
		.callback = alarm_handler,
		.flags = COUNTER_ALARM_CFG_ABSOLUTE |
			 COUNTER_ALARM_CFG_EXPIRE_WHEN_LATE,
		.user_data = NULL
	};

	err = counter_set_guard_period(dev, guard,
					COUNTER_GUARD_PERIOD_LATE_TO_SET);
	zassert_equal(0, err, "%s: Unexpected error", dev->name);

	err = counter_start(dev);
	zassert_equal(0, err, "%s: Unexpected error", dev->name);

	k_busy_wait(2*tick_us);

	alarm_cfg.ticks = 0;
	err = counter_set_channel_alarm(dev, 0, &alarm_cfg);
	zassert_equal(-ETIME, err, "%s: Unexpected error (%d)", dev->name, err);

	/* wait couple of ticks */
	k_busy_wait(5*tick_us);

	cnt = IS_ENABLED(CONFIG_ZERO_LATENCY_IRQS) ?
		alarm_cnt : k_sem_count_get(&alarm_cnt_sem);
	zassert_equal(1, cnt,
			"%s: Expected %d callbacks, got %d\n",
			dev->name, 1, cnt);

	err = counter_get_value(dev, &(alarm_cfg.ticks));
	zassert_true(err == 0, "%s: Counter read failed (err: %d)", dev->name,
		     err);

	err = counter_set_channel_alarm(dev, 0, &alarm_cfg);
	zassert_equal(-ETIME, err, "%s: Failed to set an alarm (err: %d)",
			dev->name, err);

	/* wait to ensure that tick+1 timeout will expire. */
	k_busy_wait(3*tick_us);

	cnt = IS_ENABLED(CONFIG_ZERO_LATENCY_IRQS) ?
		alarm_cnt : k_sem_count_get(&alarm_cnt_sem);
	zassert_equal(2, cnt,
			"%s: Expected %d callbacks, got %d\n",
			dev->name, 2, cnt);
}

static void test_late_alarm_error_instance(const struct device *dev)
{
	int err;
	uint32_t tick_us = (uint32_t)counter_ticks_to_us(dev, 1);
	uint32_t guard = counter_us_to_ticks(dev, 200);
	struct counter_alarm_cfg alarm_cfg = {
		.callback = alarm_handler,
		.flags = COUNTER_ALARM_CFG_ABSOLUTE,
		.user_data = NULL
	};

	err = counter_set_guard_period(dev, guard,
					COUNTER_GUARD_PERIOD_LATE_TO_SET);
	zassert_equal(0, err, "%s: Unexpected error", dev->name);

	err = counter_start(dev);
	zassert_equal(0, err, "%s: Unexpected error", dev->name);

	k_busy_wait(2*tick_us);

	alarm_cfg.ticks = 0;
	err = counter_set_channel_alarm(dev, 0, &alarm_cfg);
	zassert_equal(-ETIME, err,
			"%s: Failed to detect late setting (err: %d)",
			dev->name, err);

	err = counter_get_value(dev, &(alarm_cfg.ticks));
	zassert_true(err == 0, "%s: Counter read failed (err: %d)", dev->name,
		     err);

	err = counter_set_channel_alarm(dev, 0, &alarm_cfg);
	zassert_equal(-ETIME, err,
			"%s: Counter failed to detect late setting (err: %d)",
			dev->name, err);
}

static bool late_detection_capable(const struct device *dev)
{
	uint32_t guard = counter_get_guard_period(dev,
					COUNTER_GUARD_PERIOD_LATE_TO_SET);
	int err = counter_set_guard_period(dev, guard,
					COUNTER_GUARD_PERIOD_LATE_TO_SET);

	if (err == -ENOTSUP) {
		return false;
	}

	if (single_channel_alarm_capable(dev) == false) {
		return false;
	}

	return true;
}

ZTEST(counter_basic, test_late_alarm)
{
	test_all_instances(test_late_alarm_instance, late_detection_capable);
}

ZTEST(counter_basic, test_late_alarm_error)
{
	test_all_instances(test_late_alarm_error_instance,
			   late_detection_capable);
}

static void test_short_relative_alarm_instance(const struct device *dev)
{
	int err;
	uint32_t cnt;
	uint32_t tick_us = (uint32_t)counter_ticks_to_us(dev, 1);
	struct counter_alarm_cfg alarm_cfg = {
		.callback = alarm_handler,
		.flags = 0,
		.user_data = NULL
	};

	/* for timers with very short ticks, counter_ticks_to_us() returns 0 */
	tick_us = tick_us == 0 ? 1 : tick_us;

	err = counter_start(dev);
	zassert_equal(0, err, "%s: Unexpected error", dev->name);

	if (IS_ENABLED(CONFIG_COUNTER_NRF_RTC)) {
		k_busy_wait(1000);
	}

	alarm_cfg.ticks = 1;

	for (int i = 0; i < 100; ++i) {
		err = counter_set_channel_alarm(dev, 0, &alarm_cfg);
		zassert_equal(0, err,
				"%s: Failed to set an alarm (err: %d)",
				dev->name, err);

		/* wait to ensure that tick+1 timeout will expire. */
		k_busy_wait(3*tick_us);

		cnt = IS_ENABLED(CONFIG_ZERO_LATENCY_IRQS) ?
			alarm_cnt : k_sem_count_get(&alarm_cnt_sem);
		zassert_equal(i + 1, cnt,
				"%s: Expected %d callbacks, got %d\n",
				dev->name, i + 1, cnt);
	}
}

/* Function checks if relative alarm set for 1 tick will expire. If handler is
 * not called within near future it indicates that driver do not support it and
 * more extensive testing is skipped.
 */
static bool short_relative_capable(const struct device *dev)
{
	struct counter_alarm_cfg alarm_cfg = {
		.callback = alarm_handler,
		.flags = 0,
		.user_data = NULL,
		.ticks = 1
	};
	int err;
	uint32_t cnt;
	bool ret;

	if (single_channel_alarm_capable(dev) == false) {
		return false;
	}

	err = counter_start(dev);
	if (err != 0) {
		ret = false;
		goto end;
	}

	k_sem_reset(&alarm_cnt_sem);
	alarm_cnt = 0;
	err = counter_set_channel_alarm(dev, 0, &alarm_cfg);
	if (err != 0) {
		ret = false;
		goto end;
	}

	k_busy_wait(counter_ticks_to_us(dev, 10));
	cnt = IS_ENABLED(CONFIG_ZERO_LATENCY_IRQS) ?
			alarm_cnt : k_sem_count_get(&alarm_cnt_sem);
	if (cnt == 1) {
		ret = true;
	} else {
		ret = false;
		(void)counter_cancel_channel_alarm(dev, 0);
	}

end:
	k_sem_reset(&alarm_cnt_sem);
	alarm_cnt = 0;
	counter_stop(dev);
	k_busy_wait(1000);

	return ret;
}

ZTEST(counter_basic, test_short_relative_alarm)
{
	test_all_instances(test_short_relative_alarm_instance,
			short_relative_capable);
}

/* Test checks if cancelled alarm does not get triggered when new alarm is
 * configured at the point where previous alarm was about to expire.
 */
static void test_cancelled_alarm_does_not_expire_instance(const struct device *dev)
{
	int err;
	uint32_t cnt;
	uint32_t us = 1000;
	uint32_t ticks = counter_us_to_ticks(dev, us);
	uint32_t top = counter_get_top_value(dev);

	us = (uint32_t)counter_ticks_to_us(dev, ticks);

	struct counter_alarm_cfg alarm_cfg = {
		.callback = alarm_handler,
		.flags = COUNTER_ALARM_CFG_ABSOLUTE,
		.user_data = NULL
	};

	err = counter_start(dev);
	zassert_equal(0, err, "%s: Unexpected error", dev->name);


	for (int i = 0; i < us/2; ++i) {
		err = counter_get_value(dev, &(alarm_cfg.ticks));
		zassert_true(err == 0, "%s: Counter read failed (err: %d)",
			     dev->name, err);

		alarm_cfg.ticks	+= ticks;
		alarm_cfg.ticks = alarm_cfg.ticks % top;
		err = counter_set_channel_alarm(dev, 0, &alarm_cfg);
		zassert_equal(0, err, "%s: Failed to set an alarm (err: %d)",
				dev->name, err);

		err = counter_cancel_channel_alarm(dev, 0);
		zassert_equal(0, err, "%s: Failed to cancel an alarm (err: %d)",
				dev->name, err);

		k_busy_wait(us/2 + i);

		alarm_cfg.ticks = alarm_cfg.ticks + 2*ticks;
		alarm_cfg.ticks = alarm_cfg.ticks % top;
		err = counter_set_channel_alarm(dev, 0, &alarm_cfg);
		zassert_equal(0, err, "%s: Failed to set an alarm (err: %d)",
				dev->name, err);

		/* wait to ensure that tick+1 timeout will expire. */
		k_busy_wait(us);

		err = counter_cancel_channel_alarm(dev, 0);
		zassert_equal(0, err, "%s: Failed to cancel an alarm (err: %d)",
					dev->name, err);

		cnt = IS_ENABLED(CONFIG_ZERO_LATENCY_IRQS) ?
			alarm_cnt : k_sem_count_get(&alarm_cnt_sem);
		zassert_equal(0, cnt,
				"%s: Expected %d callbacks, got %d (i:%d)\n",
				dev->name, 0, cnt, i);
	}
}

static bool reliable_cancel_capable(const struct device *dev)
{
	/* Test performed only for NRF_RTC instances. Other probably will fail.
	 */
#if defined(CONFIG_COUNTER_NRF_RTC) || defined(CONFIG_COUNTER_NRF_TIMER)
	return true;
#endif
#ifdef CONFIG_COUNTER_TIMER_STM32
	if (single_channel_alarm_capable(dev)) {
		return true;
	}
#endif
#ifdef CONFIG_COUNTER_TIMER_GD32
	if (single_channel_alarm_capable(dev)) {
		return true;
	}
#endif
#ifdef CONFIG_COUNTER_NATIVE_POSIX
	if (dev == DEVICE_DT_GET(DT_NODELABEL(counter0))) {
		return true;
	}
#endif
#ifdef CONFIG_COUNTER_NXP_S32_SYS_TIMER
	if (single_channel_alarm_capable(dev)) {
		return true;
	}
#endif
	return false;
}

ZTEST(counter_basic, test_cancelled_alarm_does_not_expire)
{
	test_all_instances(test_cancelled_alarm_does_not_expire_instance,
			reliable_cancel_capable);
}

static void *counter_setup(void)
{
	int i;

	/* Give required clocks some time to stabilize. In particular, nRF SoCs
	 * need such delay for the Xtal LF clock source to start and for this
	 * test to use the correct timing.
	 */
	k_busy_wait(USEC_PER_MSEC * 300);

	k_sem_init(&top_cnt_sem, 0, UINT_MAX);
	k_object_access_grant(&top_cnt_sem, k_current_get());

	k_sem_init(&alarm_cnt_sem, 0, UINT_MAX);
	k_object_access_grant(&alarm_cnt_sem, k_current_get());

	for (i = 0; i < ARRAY_SIZE(devices); i++) {
		zassert_true(device_is_ready(devices[i]),
			     "Device %s is not ready", devices[i]->name);
		k_object_access_grant(devices[i], k_current_get());
	}

	return NULL;
}

/* Uses callbacks, run in supervisor mode */
ZTEST_SUITE(counter_basic, NULL, counter_setup, NULL, NULL, NULL);

/* No callbacks, run in usermode */
ZTEST_SUITE(counter_no_callback, NULL, counter_setup, NULL, NULL, NULL);
