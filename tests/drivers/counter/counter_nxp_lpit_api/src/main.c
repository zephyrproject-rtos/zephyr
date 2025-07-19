/*
 * Copyright (c) 2018, Nordic Semiconductor ASA
 * Copyright 2024-2025 NXP
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
#define DEVS_FOR_DT_COMPAT(compat)       DT_FOREACH_STATUS_OKAY(compat, DEVICE_DT_GET_AND_COMMA)

static const struct device *const devices[] = {
#ifdef CONFIG_COUNTER_MCUX_LPIT
	DEVS_FOR_DT_COMPAT(nxp_lpit_channel)
#endif
};

typedef void (*counter_test_func_t)(const struct device *dev);
typedef bool (*counter_capability_func_t)(const struct device *dev);

static inline uint32_t get_counter_period_us(const struct device *dev)
{
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
	struct counter_top_cfg top_cfg = {.callback = NULL, .user_data = NULL, .flags = 0};

	top_cfg.ticks = counter_get_max_top_value(dev);
	err = counter_set_top_value(dev, &top_cfg);
	if (err == -ENOTSUP) {
		/* If resetting is not support, attempt without reset. */
		top_cfg.flags = COUNTER_TOP_CFG_DONT_RESET;
		err = counter_set_top_value(dev, &top_cfg);
	}
	zassert_true((err == 0) || (err == -ENOTSUP), "%s: Setting top value to default failed",
		     dev->name);

	err = counter_stop(dev);
	zassert_equal(0, err, "%s: Counter failed to stop", dev->name);
}

static void test_all_instances(counter_test_func_t func, counter_capability_func_t capability_check)
{
	int devices_skipped = 0;

	zassert_true(ARRAY_SIZE(devices) > 0, "No device found");
	for (int i = 0; i < ARRAY_SIZE(devices); i++) {
		counter_setup_instance(devices[i]);
		if ((capability_check == NULL) || capability_check(devices[i])) {
			TC_PRINT("Testing %s\n", devices[i]->name);
			func(devices[i]);
		} else {
			TC_PRINT("Skipped for %s\n", devices[i]->name);
			devices_skipped++;
		}
		counter_tear_down_instance(devices[i]);
		/* Allow logs to be printed. */
		k_sleep(K_MSEC(100));
	}
	if (devices_skipped == ARRAY_SIZE(devices)) {
		ztest_test_skip();
	}
}

static bool set_top_value_capable(const struct device *dev)
{
	struct counter_top_cfg cfg = {.ticks = counter_get_top_value(dev) - 1};
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
	zassert_true(user_data == exp_user_data, "%s: Unexpected callback", dev->name);
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
		.callback = top_handler, .user_data = exp_user_data, .flags = 0};

	k_sem_reset(&top_cnt_sem);
	top_cnt = 0;

	counter_period_us = get_counter_period_us(dev);
	top_cfg.ticks = counter_us_to_ticks(dev, counter_period_us);
	err = counter_start(dev);
	zassert_equal(0, err, "%s: Counter failed to start", dev->name);

	k_busy_wait(5000);

	err = counter_get_value(dev, &cnt);
	zassert_true(err == 0, "%s: Counter read failed (err: %d)", dev->name, err);
	if (counter_is_counting_up(dev)) {
		err = (cnt > 0) ? 0 : 1;
	} else {
		top_value = counter_get_top_value(dev);
		err = (cnt < top_value) ? 0 : 1;
	}
	zassert_true(err == 0, "%s: Counter should progress", dev->name);

	err = counter_set_top_value(dev, &top_cfg);
	zassert_equal(0, err, "%s: Counter failed to set top value (err: %d)", dev->name, err);

	k_busy_wait(5.2 * counter_period_us);

	top_handler_cnt =
		IS_ENABLED(CONFIG_ZERO_LATENCY_IRQS) ? top_cnt : k_sem_count_get(&top_cnt_sem);
	zassert_true(top_handler_cnt == 5U, "%s: Unexpected number of turnarounds (%d).", dev->name,
		     top_handler_cnt);
}

ZTEST(counter_basic, test_set_top_value_with_alarm)
{
	test_all_instances(test_set_top_value_with_alarm_instance, set_top_value_capable);
}

static void test_set_top_value_without_alarm_instance(const struct device *dev)
{
	int err;
	uint32_t cnt;
	uint32_t top_value;
	uint32_t counter_period_us;
	struct counter_top_cfg top_cfg = {.callback = NULL, .user_data = NULL, .flags = 0};

	counter_period_us = get_counter_period_us(dev);
	top_cfg.ticks = counter_us_to_ticks(dev, counter_period_us);
	err = counter_start(dev);
	zassert_equal(0, err, "%s: Counter failed to start", dev->name);

	k_busy_wait(5000);

	err = counter_get_value(dev, &cnt);
	zassert_true(err == 0, "%s: Counter read failed (err: %d)", dev->name, err);
	if (counter_is_counting_up(dev)) {
		err = (cnt > 0) ? 0 : 1;
	} else {
		top_value = counter_get_top_value(dev);
		err = (cnt < top_value) ? 0 : 1;
	}
	zassert_true(err == 0, "%s: Counter should progress", dev->name);

	err = counter_set_top_value(dev, &top_cfg);
	zassert_equal(0, err, "%s: Counter failed to set top value (err: %d)", dev->name, err);

	zassert_true(counter_get_top_value(dev) == top_cfg.ticks, "%s: new top value not in use.",
		     dev->name);
}

ZTEST_USER(counter_no_callback, test_set_top_value_without_alarm)
{
	test_all_instances(test_set_top_value_without_alarm_instance, set_top_value_capable);
}

static void alarm_handler(const struct device *dev, uint8_t chan_id, uint32_t counter,
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
	zassert_true(err == 0, "%s: Counter read failed (err: %d)", dev->name, err);

	top = counter_get_top_value(dev);
	if (counter_is_counting_up(dev)) {
		diff = (now < counter) ? (now + top - counter) : (now - counter);
	} else {
		diff = (now > counter) ? (counter + top - now) : (counter - now);
	}

	zassert_true(diff <= counter_us_to_ticks(dev, processing_limit_us),
		     "Unexpected distance between reported alarm value(%u) "
		     "and actual counter value (%u), top:%d (processing "
		     "time limit (%d us) might be exceeded?",
		     counter, now, top, (int)processing_limit_us);

	if (user_data) {
		zassert_true(&cntr_alarm_cfg == user_data, "%s: Unexpected callback", dev->name);
	}

	if (IS_ENABLED(CONFIG_ZERO_LATENCY_IRQS)) {
		alarm_cnt++;
		return;
	}
	zassert_true(k_is_in_isr(), "%s: Expected interrupt context", dev->name);
	k_sem_give(&alarm_cnt_sem);
}

static void test_single_shot_alarm_instance(const struct device *dev, bool set_top)
{
	int err;
	uint32_t ticks;
	uint32_t cnt;
	uint32_t counter_period_us;
	uint8_t chan_id = 0, n = 10;
	struct counter_top_cfg top_cfg = {
		.callback = top_handler, .user_data = exp_user_data, .flags = 0};

	counter_period_us = get_counter_period_us(dev);
	ticks = counter_us_to_ticks(dev, counter_period_us);
	top_cfg.ticks = ticks;

	/* This case tests alarm with specific ticks, for LPIT, it counts down from timer value to 0
	 * then trigger interrupt, hence it need to count down from ticks, set flag as
	 * COUNTER_ALARM_CFG_ABSOLUTE
	 */
	cntr_alarm_cfg.flags = COUNTER_ALARM_CFG_ABSOLUTE;

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

	for (int j = 0; j < n; j++) {
		err = counter_cancel_channel_alarm(dev, j);
		if (err == 0) {
			chan_id = j;
			break;
		}
	}
	zassert_not_equal(10, chan_id, "%s: Failed to set an alarm", dev->name);

	if (set_top) {
		err = counter_set_top_value(dev, &top_cfg);

		zassert_equal(0, err, "%s: Counter failed to set top value", dev->name);

		cntr_alarm_cfg.ticks = ticks + 1;
		err = counter_set_channel_alarm(dev, chan_id, &cntr_alarm_cfg);
		zassert_equal(-EINVAL, err,
			      "%s: Counter should return error because ticks"
			      " exceeded the limit set alarm",
			      dev->name);
		cntr_alarm_cfg.ticks = ticks - 1;
	}

	cntr_alarm_cfg.ticks = ticks;
	err = counter_set_channel_alarm(dev, chan_id, &cntr_alarm_cfg);
	zassert_equal(0, err, "%s: Counter set alarm failed (err: %d)", dev->name, err);

	k_busy_wait(2 * (uint32_t)counter_ticks_to_us(dev, ticks));

	cnt = IS_ENABLED(CONFIG_ZERO_LATENCY_IRQS) ? alarm_cnt : k_sem_count_get(&alarm_cnt_sem);
	zassert_equal(1, cnt, "%s: Expecting 1 alarm callback, get %d", dev->name, cnt);

	k_busy_wait(1.5 * counter_ticks_to_us(dev, ticks));
	cnt = IS_ENABLED(CONFIG_ZERO_LATENCY_IRQS) ? alarm_cnt : k_sem_count_get(&alarm_cnt_sem);
	zassert_equal(1, cnt, "%s: Expecting alarm callback", dev->name);

	err = counter_cancel_channel_alarm(dev, chan_id);
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
	zassert_true((err == 0) || (err == -ENOTSUP), "%s: Setting top value to default failed",
		     dev->name);

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
	return single_channel_alarm_capable(dev) && set_top_value_capable(dev);
}

ZTEST(counter_basic, test_single_shot_alarm_notop)
{
	test_all_instances(test_single_shot_alarm_notop_instance, single_channel_alarm_capable);
}

ZTEST(counter_basic, test_single_shot_alarm_top)
{
	test_all_instances(test_single_shot_alarm_top_instance,
			   single_channel_alarm_and_custom_top_capable);
}

static void *clbk_data[10];

static void alarm_handler2(const struct device *dev, uint8_t chan_id, uint32_t counter,
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

static void test_all_channels_instance(const struct device *dev)
{
	int err;
	const int n = 10;
	uint8_t chan_id = 0;
	struct counter_alarm_cfg alarm_cfgs;
	uint32_t ticks;
	uint32_t cnt;
	uint32_t counter_period_us;

	counter_period_us = get_counter_period_us(dev);
	ticks = counter_us_to_ticks(dev, counter_period_us);

	/* This case tests alarm with specific ticks, for LPIT, it counts down from timer value to 0
	 * then trigger interrupt, hence it need to count down from ticks, set flag as
	 * COUNTER_ALARM_CFG_ABSOLUTE
	 */
	alarm_cfgs.flags = COUNTER_ALARM_CFG_ABSOLUTE;
	alarm_cfgs.ticks = ticks;
	alarm_cfgs.callback = alarm_handler2;
	alarm_cfgs.user_data = NULL;

	err = counter_start(dev);
	zassert_equal(0, err, "%s: Counter failed to start", dev->name);

	for (int i = 0; i < n; i++) {
		err = counter_set_channel_alarm(dev, i, &alarm_cfgs);
		if (err == 0) {
			chan_id = i;
			break;
		}
	}
	zassert_equal(0, err, "%s: Unexpected error on setting alarm", dev->name);

	k_busy_wait(1.5 * counter_ticks_to_us(dev, ticks));
	cnt = IS_ENABLED(CONFIG_ZERO_LATENCY_IRQS) ? alarm_cnt : k_sem_count_get(&alarm_cnt_sem);
	zassert_equal(1, cnt, "%s: Expecting alarm callback %d, actually get: %d", dev->name,
		      chan_id, cnt);

	err = counter_cancel_channel_alarm(dev, chan_id);
	zassert_equal(0, err, "%s: Unexpected error on disabling alarm", dev->name);

	err = counter_cancel_channel_alarm(dev, chan_id + 1);
	zassert_equal(-ENOTSUP, err, "%s: Unexpected error on disabling alarm", dev->name);
}

ZTEST(counter_basic, test_all_channels)
{
	test_all_instances(test_all_channels_instance, single_channel_alarm_capable);
}

static void test_valid_function_without_alarm(const struct device *dev)
{
	int err;
	uint32_t ticks;
	uint32_t ticks_expected;
	uint32_t tick_current;
	uint32_t ticks_tol;
	uint32_t wait_for_us;
	uint32_t freq = counter_get_frequency(dev);

	/* For timers which cannot count to at least 2 ms before overflow
	 * the test is skipped by test_all_instances function because sufficient
	 * accuracy of the test cannot be achieved.
	 */

	zassert_true(freq != 0, "%s: counter could not get frequency", dev->name);

	/* Set time of counting based on counter frequency to
	 * ensure convenient run time and accuracy of the test.
	 */
	if (freq < 1000) {
		/* Ensure to have 1 tick for 1 sec clock */
		wait_for_us = 1100000;
		ticks_expected = counter_us_to_ticks(dev, wait_for_us);
	} else if (freq < 1000000) {
		/* Calculate wait time for convenient ticks count */
		ticks_expected = 1000;
		wait_for_us = (ticks_expected * 1000000) / freq;
	} else {
		/* Wait long enough for high frequency clocks to minimize
		 * impact of latencies and k_busy_wait function accuracy.
		 */
		wait_for_us = 1000;
		ticks_expected = counter_us_to_ticks(dev, wait_for_us);
	}

	/* Set 10% or 2 ticks tolerance, whichever is greater */
	ticks_tol = ticks_expected / 10;
	ticks_tol = ticks_tol < 2 ? 2 : ticks_tol;

	err = counter_start(dev);
	zassert_equal(0, err, "%s: counter failed to start", dev->name);

	/* counter might not start from 0, use current value as offset */
	counter_get_value(dev, &tick_current);
	if (counter_is_counting_up(dev)) {
		ticks_expected += tick_current;
	} else {
		ticks_expected = tick_current - ticks_expected;
	}

	k_busy_wait(wait_for_us);

	err = counter_get_value(dev, &ticks);

	zassert_equal(0, err, "%s: could not get counter value", dev->name);
	zassert_between_inclusive(
		ticks, ticks_expected > ticks_tol ? ticks_expected - ticks_tol : 0,
		ticks_expected + ticks_tol, "%s: counter ticks not in tolerance", dev->name);

	/* ticks count is always within ticks_tol for RTC, therefor
	 * check, if ticks are greater than 0.
	 */
	zassert_true((ticks > 0), "%s: counter did not count", dev->name);

	err = counter_stop(dev);
	zassert_equal(0, err, "%s: counter failed to stop", dev->name);
}

static bool ms_period_capable(const struct device *dev)
{
	uint32_t freq_khz;
	uint32_t max_time_ms;

	/* Assume 2 ms counter periode can be set for frequency below 1 kHz*/
	if (counter_get_frequency(dev) < 1000) {
		return true;
	}

	freq_khz = counter_get_frequency(dev) / 1000;
	max_time_ms = counter_get_top_value(dev) / freq_khz;

	if (max_time_ms >= 2) {
		return true;
	}

	return false;
}

ZTEST(counter_basic, test_valid_function_without_alarm)
{
	test_all_instances(test_valid_function_without_alarm, ms_period_capable);
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
	uint8_t chan_id = 0, n = 10;

	us = (uint32_t)counter_ticks_to_us(dev, ticks);

	struct counter_alarm_cfg alarm_cfg = {
		.callback = alarm_handler, .flags = COUNTER_ALARM_CFG_ABSOLUTE, .user_data = NULL};

	k_sem_reset(&alarm_cnt_sem);
	alarm_cnt = 0;

	err = counter_start(dev);
	zassert_equal(0, err, "%s: Unexpected error", dev->name);

	for (int i = 0; i < us / 2; ++i) {
		err = counter_get_value(dev, &(alarm_cfg.ticks));
		zassert_true(err == 0, "%s: Counter read failed (err: %d)", dev->name, err);

		alarm_cfg.ticks += ticks;
		alarm_cfg.ticks = alarm_cfg.ticks % top;

		for (int j = 0; j < n; j++) {
			err = counter_set_channel_alarm(dev, j, &alarm_cfg);
			if (err == 0) {
				chan_id = j;
				break;
			}
		}

		zassert_not_equal(10, chan_id, "%s: Failed to set an alarm", dev->name);

		err = counter_cancel_channel_alarm(dev, chan_id);
		zassert_equal(0, err, "%s: Failed to cancel an alarm (err: %d)", dev->name, err);

		k_busy_wait(us / 2 + i);

		alarm_cfg.ticks = alarm_cfg.ticks + 2 * ticks;
		alarm_cfg.ticks = alarm_cfg.ticks % top;
		err = counter_set_channel_alarm(dev, chan_id, &alarm_cfg);
		zassert_equal(0, err, "%s: Failed to set an alarm (err: %d)", dev->name, err);

		/* wait to ensure that tick+1 timeout will expire. */
		k_busy_wait(us);

		err = counter_cancel_channel_alarm(dev, chan_id);
		zassert_equal(0, err, "%s: Failed to cancel an alarm (err: %d)", dev->name, err);

		cnt = IS_ENABLED(CONFIG_ZERO_LATENCY_IRQS) ? alarm_cnt
							   : k_sem_count_get(&alarm_cnt_sem);
		zassert_equal(0, cnt, "%s: Expected %d callbacks, got %d (i:%d)\n", dev->name, 0,
			      cnt, i);
	}
}

static bool reliable_cancel_capable(const struct device *dev)
{
#ifdef CONFIG_COUNTER_MCUX_LPIT
	if (single_channel_alarm_capable(dev)) {
		return true;
	}
#endif
	return false;
}

ZTEST(counter_basic, test_cancelled_alarm_does_not_expire)
{
	test_all_instances(test_cancelled_alarm_does_not_expire_instance, reliable_cancel_capable);
}

static void *counter_setup(void)
{
	/* Give required clocks some time to stabilize. In particular, nRF SoCs
	 * need such delay for the Xtal LF clock source to start and for this
	 * test to use the correct timing.
	 */
	k_busy_wait(USEC_PER_MSEC * 300);

	k_sem_init(&top_cnt_sem, 0, UINT_MAX);
	k_object_access_grant(&top_cnt_sem, k_current_get());

	k_sem_init(&alarm_cnt_sem, 0, UINT_MAX);
	k_object_access_grant(&alarm_cnt_sem, k_current_get());

	for (int i = 0; i < ARRAY_SIZE(devices); i++) {
		zassert_true(device_is_ready(devices[i]), "Device %s is not ready",
			     devices[i]->name);
		k_object_access_grant(devices[i], k_current_get());
	}

	return NULL;
}

/* Uses callbacks, run in supervisor mode */
ZTEST_SUITE(counter_basic, NULL, counter_setup, NULL, NULL, NULL);

/* No callbacks, run in usermode */
ZTEST_SUITE(counter_no_callback, NULL, counter_setup, NULL, NULL, NULL);
