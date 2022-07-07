/*
 * Copyright (c) 2018, Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/counter.h>
#include <zephyr/drivers/rtc/maxim_ds3231.h>
#include <ztest.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(test);

static struct k_sem top_cnt_sem;
static struct k_sem alarm_cnt_sem;
static struct k_poll_signal sync_sig;

static void top_handler(const struct device *dev, void *user_data);

void *exp_user_data = (void *)199;

#define COUNTER_PERIOD_US USEC_PER_SEC

struct counter_alarm_cfg alarm_cfg;
struct counter_alarm_cfg alarm_cfg2;

static const char *const devices[] = {
	DT_LABEL(DT_NODELABEL(ds3231)),
};
typedef void (*counter_test_func_t)(const char *dev_name);

typedef bool (*counter_capability_func_t)(const char *dev_name);


static void counter_setup_instance(const char *dev_name)
{
	k_sem_reset(&alarm_cnt_sem);
}

static void counter_tear_down_instance(const char *dev_name)
{
	int err;
	const struct device *dev;
	struct counter_top_cfg top_cfg = {
		.callback = NULL,
		.user_data = NULL,
		.flags = 0
	};

	dev = device_get_binding(dev_name);

	top_cfg.ticks = counter_get_max_top_value(dev);
	err = counter_set_top_value(dev, &top_cfg);
	if (err == -ENOTSUP) {
		/* If resetting is not support, attempt without reset. */
		top_cfg.flags = COUNTER_TOP_CFG_DONT_RESET;
		err = counter_set_top_value(dev, &top_cfg);

	}
	zassert_true((err == 0) || (err == -ENOTSUP),
		     "%s: Setting top value to default failed", dev_name);

	err = counter_stop(dev);
	/* DS3231 counter cannot be stopped */
	zassert_equal(-ENOTSUP, err, "%s: Counter failed to stop", dev_name);

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
	const struct device *dev = device_get_binding(dev_name);
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
	k_sem_give(&top_cnt_sem);
}

void test_set_top_value_with_alarm_instance(const char *dev_name)
{
	const struct device *dev;
	int err;
	uint32_t cnt;
	uint32_t top_cnt;
	struct counter_top_cfg top_cfg = {
		.callback = top_handler,
		.user_data = exp_user_data,
		.flags = 0
	};

	k_sem_reset(&top_cnt_sem);

	dev = device_get_binding(dev_name);
	top_cfg.ticks = counter_us_to_ticks(dev, COUNTER_PERIOD_US);
	err = counter_start(dev);
	zassert_equal(0, err, "%s: Counter failed to start", dev_name);

	k_busy_wait(5000);

	err = counter_get_value(dev, &cnt);
	zassert_true(err == 0, "%s: Counter read failed (err: %d)", dev_name,
		     err);
	if (counter_is_counting_up(dev)) {
		err = (cnt > 0) ? 0 : 1;
	} else {
		top_cnt = counter_get_top_value(dev);
		err = (cnt < top_cnt) ? 0 : 1;
	}
	zassert_true(err == 0, "%s: Counter should progress", dev_name);

	err = counter_set_top_value(dev, &top_cfg);
	zassert_equal(0, err, "%s: Counter failed to set top value (err: %d)",
		      dev_name, err);

	k_sleep(K_USEC(5.2 * COUNTER_PERIOD_US));

	top_cnt = k_sem_count_get(&top_cnt_sem);
	zassert_true(top_cnt == 5U,
		     "%s: Unexpected number of turnarounds (%d).",
		     dev_name, top_cnt);
}

void test_set_top_value_with_alarm(void)
{
	test_all_instances(test_set_top_value_with_alarm_instance,
			   set_top_value_capable);
}

void test_set_top_value_without_alarm_instance(const char *dev_name)
{
	const struct device *dev;
	int err;
	uint32_t cnt;
	uint32_t top_cnt;
	struct counter_top_cfg top_cfg = {
		.callback = NULL,
		.user_data = NULL,
		.flags = 0
	};

	dev = device_get_binding(dev_name);
	top_cfg.ticks = counter_us_to_ticks(dev, COUNTER_PERIOD_US);
	err = counter_start(dev);
	zassert_equal(0, err, "%s: Counter failed to start", dev_name);

	k_sleep(K_USEC(5000));

	err = counter_get_value(dev, &cnt);
	zassert_true(err == 0, "%s: Counter read failed (err: %d)", dev_name,
		     err);
	if (counter_is_counting_up(dev)) {
		err = (cnt > 0) ? 0 : 1;
	} else {
		top_cnt = counter_get_top_value(dev);
		err = (cnt < top_cnt) ? 0 : 1;
	}
	zassert_true(err == 0, "%s: Counter should progress", dev_name);

	err = counter_set_top_value(dev, &top_cfg);
	zassert_equal(0, err, "%s: Counter failed to set top value (err: %d)",
		      dev_name, err);

	zassert_true(counter_get_top_value(dev) == top_cfg.ticks,
		     "%s: new top value not in use.",
		     dev_name);
}

void test_set_top_value_without_alarm(void)
{
	test_all_instances(test_set_top_value_without_alarm_instance,
			   set_top_value_capable);
}

static void alarm_handler(const struct device *dev, uint8_t chan_id,
			  uint32_t counter,
			  void *user_data)
{
	uint32_t now;
	int err;

	err = counter_get_value(dev, &now);
	zassert_true(err == 0, "%s: Counter read failed (err: %d)",
		     dev->name, err);

	if (counter_is_counting_up(dev)) {
		zassert_true(now >= counter,
			     "%s: Alarm (%d) too early now: %d (counting up).",
			     dev->name, counter, now);
	} else {
		zassert_true(now <= counter,
			     "%s: Alarm (%d) too early now: %d (counting down).",
			     dev->name, counter, now);
	}

	if (user_data) {
		zassert_true(&alarm_cfg == user_data,
			     "%s: Unexpected callback", dev->name);
	}

	/* DS3231 does not invoke callbacks from interrupt context. */
	zassert_false(k_is_in_isr(), "%s: Unexpected interrupt context",
		      dev->name);
	k_sem_give(&alarm_cnt_sem);
}

void test_single_shot_alarm_instance(const char *dev_name, bool set_top)
{
	const struct device *dev;
	int err;
	uint32_t ticks;
	uint32_t alarm_cnt;
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

	k_sem_reset(&alarm_cnt_sem);

	if (counter_get_num_of_channels(dev) < 1U) {
		/* Counter does not support any alarm */
		return;
	}

	err = counter_start(dev);
	zassert_equal(-EALREADY, err, "%s: Counter failed to start", dev_name);

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

	err = counter_set_top_value(dev, &top_cfg);
	k_sleep(K_USEC(2 * (uint32_t)counter_ticks_to_us(dev, ticks)));

	alarm_cnt = k_sem_count_get(&alarm_cnt_sem);
	zassert_equal(1, alarm_cnt,
		      "%s: Expecting alarm callback", dev_name);

	k_sleep(K_USEC(1.5 * counter_ticks_to_us(dev, ticks)));
	alarm_cnt = k_sem_count_get(&alarm_cnt_sem);
	zassert_equal(1, alarm_cnt,
		      "%s: Expecting alarm callback", dev_name);

	err = counter_cancel_channel_alarm(dev, 0);
	zassert_equal(0, err, "%s: Counter disabling alarm failed %d", dev_name, err);

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
		     "%s: Setting top value to default failed", dev_name);

	err = counter_stop(dev);
	/* DS3231 counter cannot be stopped */
	zassert_equal(-ENOTSUP, err, "%s: Counter failed to stop", dev_name);
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
	const struct device *dev = device_get_binding(dev_name);

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

static void alarm_handler2(const struct device *dev, uint8_t chan_id,
			   uint32_t counter,
			   void *user_data)
{
	clbk_data[k_sem_count_get(&alarm_cnt_sem)] = user_data;
	k_sem_give(&alarm_cnt_sem);
}

/*
 * Two alarms set. First alarm is absolute, second relative. Because
 * setting of both alarms is delayed it is expected that second alarm
 * will expire first (relative to the time called) while first alarm
 * will expire after next wrap around.
 */
void test_multiple_alarms_instance(const char *dev_name)
{
	const struct device *dev;
	int err;
	uint32_t ticks;
	uint32_t alarm_cnt;
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

	k_sem_reset(&alarm_cnt_sem);

	if (counter_get_num_of_channels(dev) < 2U) {
		/* Counter does not support two alarms */
		return;
	}

	err = counter_start(dev);
	/* DS3231 is always running */
	zassert_equal(-EALREADY, err, "%s: Counter failed to start", dev_name);

	err = counter_set_top_value(dev, &top_cfg);
	zassert_equal(-ENOTSUP, err,
		      "%s: Counter failed to set top value: %d", dev_name);

	k_sleep(K_USEC(3 * (uint32_t)counter_ticks_to_us(dev, alarm_cfg.ticks)));

	err = counter_set_channel_alarm(dev, 0, &alarm_cfg);
	zassert_equal(0, err, "%s: Counter set alarm failed", dev_name);

	err = counter_set_channel_alarm(dev, 1, &alarm_cfg2);
	zassert_equal(0, err, "%s: Counter set alarm failed", dev_name);

	k_sleep(K_USEC(1.2 * counter_ticks_to_us(dev, ticks * 2U)));
	alarm_cnt = k_sem_count_get(&alarm_cnt_sem);
	zassert_equal(2, alarm_cnt,
		      "%s: Invalid number of callbacks %d (expected: %d)",
		      dev_name, alarm_cnt, 2);

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
	const struct device *dev = device_get_binding(dev_name);

	return (counter_get_num_of_channels(dev) > 1);
}

static bool not_capable(const char *dev_name)
{
	return false;
}

void test_multiple_alarms(void)
{
	/* Test not supported on DS3231 because second alarm resolution is
	 * more coarse than first alarm; code would have to be changed to
	 * align to boundaries and wait over 60 s to verify.
	 *
	 * Basic support for two channels is verified in
	 * test_all_channels_instance().
	 */
	(void)multiple_channel_alarm_capable;
	test_all_instances(test_multiple_alarms_instance,
			   not_capable);
}

void test_all_channels_instance(const char *dev_name)
{
	const struct device *dev;
	int err;
	const int n = 10;
	int nchan = 0;
	bool limit_reached = false;
	struct counter_alarm_cfg alarm_cfgs;
	uint32_t ticks;
	uint32_t alarm_cnt;

	dev = device_get_binding(dev_name);

	/* Use a delay large enough to guarantee a minute-counter
	 * rollover so both alarms can be tested.
	 */
	ticks = counter_us_to_ticks(dev, 61U * COUNTER_PERIOD_US);

	alarm_cfgs.flags = 0;
	alarm_cfgs.ticks = ticks;
	alarm_cfgs.callback = alarm_handler2;
	alarm_cfgs.user_data = NULL;

	err = counter_start(dev);
	zassert_equal(-EALREADY, err, "%s: Counter failed to start", dev_name);

	for (int i = 0; i < n; i++) {
		err = counter_set_channel_alarm(dev, i, &alarm_cfgs);
		if ((err == 0) && !limit_reached) {
			nchan++;
		} else if (err == -ENOTSUP) {
			limit_reached = true;
		} else {
			zassert_equal(0, 1,
				      "%s: Unexpected error on setting alarm: %d",
				      dev_name, err);
		}
	}

	TC_PRINT("Sleeping %u s to support minute-resolution alarm channel\n",
		 ticks + 1U);
	k_sleep(K_USEC(counter_ticks_to_us(dev, ticks + 1U)));
	alarm_cnt = k_sem_count_get(&alarm_cnt_sem);
	zassert_equal(nchan, alarm_cnt,
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
	uint32_t alarm_cnt;
	const struct device *dev = device_get_binding(dev_name);
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
	zassert_equal(0, err, "%s: Unexpected error", dev_name);

	err = counter_start(dev);
	zassert_equal(0, err, "%s: Unexpected error", dev_name);

	k_sleep(K_USEC(2 * tick_us));

	alarm_cfg.ticks = 0;
	err = counter_set_channel_alarm(dev, 0, &alarm_cfg);
	zassert_equal(-ETIME, err, "%s: Unexpected error (%d)", dev_name, err);

	/* wait couple of ticks */
	k_sleep(K_USEC(5 * tick_us));

	alarm_cnt = k_sem_count_get(&alarm_cnt_sem);
	zassert_equal(1, alarm_cnt,
		      "%s: Expected %d callbacks, got %d\n",
		      dev_name, 1, alarm_cnt);

	err = counter_get_value(dev, &(alarm_cfg.ticks));
	zassert_true(err == 0, "%s: Counter read failed (err: %d)", dev_name,
		     err);

	err = counter_set_channel_alarm(dev, 0, &alarm_cfg);
	zassert_equal(-ETIME, err, "%s: Failed to set an alarm (err: %d)",
		      dev_name, err);

	/* wait to ensure that tick+1 timeout will expire. */
	k_sleep(K_USEC(3 * tick_us));

	alarm_cnt = k_sem_count_get(&alarm_cnt_sem);
	zassert_equal(2, alarm_cnt,
		      "%s: Expected %d callbacks, got %d\n",
		      dev_name, 2, alarm_cnt);
}

void test_late_alarm_error_instance(const char *dev_name)
{
	int err;
	const struct device *dev = device_get_binding(dev_name);
	uint32_t tick_us = (uint32_t)counter_ticks_to_us(dev, 1);
	uint32_t guard = counter_us_to_ticks(dev, 200);
	struct counter_alarm_cfg alarm_cfg = {
		.callback = alarm_handler,
		.flags = COUNTER_ALARM_CFG_ABSOLUTE,
		.user_data = NULL
	};

	err = counter_set_guard_period(dev, guard,
				       COUNTER_GUARD_PERIOD_LATE_TO_SET);
	zassert_equal(0, err, "%s: Unexpected error", dev_name);

	err = counter_start(dev);
	zassert_equal(0, err, "%s: Unexpected error", dev_name);

	k_sleep(K_USEC(2 * tick_us));

	alarm_cfg.ticks = 0;
	err = counter_set_channel_alarm(dev, 0, &alarm_cfg);
	zassert_equal(-ETIME, err,
		      "%s: Failed to detect late setting (err: %d)",
		      dev_name, err);

	err = counter_get_value(dev, &(alarm_cfg.ticks));
	zassert_true(err == 0, "%s: Counter read failed (err: %d)", dev_name,
		     err);

	err = counter_set_channel_alarm(dev, 0, &alarm_cfg);
	zassert_equal(-ETIME, err,
		      "%s: Counter failed to detect late setting (err: %d)",
		      dev_name, err);
}

static bool late_detection_capable(const char *dev_name)
{
	const struct device *dev = device_get_binding(dev_name);
	uint32_t guard = counter_get_guard_period(dev,
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
	uint32_t alarm_cnt;
	const struct device *dev = device_get_binding(dev_name);
	uint32_t tick_us = (uint32_t)counter_ticks_to_us(dev, 1);
	struct counter_alarm_cfg alarm_cfg = {
		.callback = alarm_handler,
		.flags = 0,
		.user_data = NULL
	};

	err = counter_start(dev);
	zassert_equal(0, err, "%s: Unexpected error", dev_name);

	alarm_cfg.ticks = 1;

	for (int i = 0; i < 100; ++i) {
		err = counter_set_channel_alarm(dev, 0, &alarm_cfg);
		zassert_equal(0, err,
			      "%s: Failed to set an alarm (err: %d)",
			      dev_name, err);

		/* wait to ensure that tick+1 timeout will expire. */
		k_sleep(K_USEC(3 * tick_us));

		alarm_cnt = k_sem_count_get(&alarm_cnt_sem);
		zassert_equal(i + 1, alarm_cnt,
			      "%s: Expected %d callbacks, got %d\n",
			      dev_name, i + 1, alarm_cnt);
	}
}

/* Function checks if relative alarm set for 1 tick will expire. If handler is
 * not called within near future it indicates that driver do not support it and
 * more extensive testing is skipped.
 */
static bool short_relative_capable(const char *dev_name)
{
	const struct device *dev = device_get_binding(dev_name);
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

	k_sem_reset(&alarm_cnt_sem);
	err = counter_set_channel_alarm(dev, 0, &alarm_cfg);
	if (err != 0) {
		ret = false;
		goto end;
	}

	k_sleep(K_USEC(counter_ticks_to_us(dev, 10)));
	if (k_sem_count_get(&alarm_cnt_sem) == 1) {
		ret = true;
	} else {
		ret = false;
		(void)counter_cancel_channel_alarm(dev, 0);
	}

end:
	k_sem_reset(&alarm_cnt_sem);
	counter_stop(dev);
	k_sleep(K_USEC(1000));

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
	uint32_t alarm_cnt;
	const struct device *dev = device_get_binding(dev_name);
	uint32_t us = 1000;
	uint32_t ticks = counter_us_to_ticks(dev, us);

	us = (uint32_t)counter_ticks_to_us(dev, ticks);

	struct counter_alarm_cfg alarm_cfg = {
		.callback = alarm_handler,
		.flags = COUNTER_ALARM_CFG_ABSOLUTE,
		.user_data = NULL
	};

	err = counter_start(dev);
	zassert_equal(0, err, "%s: Unexpected error", dev_name);


	for (int i = 0; i < us / 2; ++i) {
		err = counter_get_value(dev, &(alarm_cfg.ticks));
		zassert_true(err == 0, "%s: Counter read failed (err: %d)",
			     dev_name, err);

		alarm_cfg.ticks += ticks;
		err = counter_set_channel_alarm(dev, 0, &alarm_cfg);
		zassert_equal(0, err, "%s: Failed to set an alarm (err: %d)",
			      dev_name, err);

		err = counter_cancel_channel_alarm(dev, 0);
		zassert_equal(0, err, "%s: Failed to cancel an alarm (err: %d)",
			      dev_name, err);

		k_sleep(K_USEC(us / 2 + i));

		alarm_cfg.ticks = alarm_cfg.ticks + 2 * alarm_cfg.ticks;
		err = counter_set_channel_alarm(dev, 0, &alarm_cfg);
		zassert_equal(0, err, "%s: Failed to set an alarm (err: %d)",
			      dev_name, err);

		/* wait to ensure that tick+1 timeout will expire. */
		k_sleep(K_USEC(us));

		err = counter_cancel_channel_alarm(dev, 0);
		zassert_equal(0, err, "%s: Failed to cancel an alarm (err: %d)",
			      dev_name, err);

		alarm_cnt = k_sem_count_get(&alarm_cnt_sem);
		zassert_equal(0, alarm_cnt,
			      "%s: Expected %d callbacks, got %d\n",
			      dev_name, 0, alarm_cnt);
	}
}

static bool reliable_cancel_capable(const char *dev_name)
{
	/* Test performed only for NRF_RTC instances. Other probably will fail.
	 */
#ifdef CONFIG_COUNTER_RTC0
	/* Nordic RTC0 may be reserved for Bluetooth */
	if (strcmp(dev_name, DT_LABEL(DT_NODELABEL(rtc0))) == 0) {
		return true;
	}
#endif

#ifdef CONFIG_COUNTER_RTC2
	if (strcmp(dev_name, DT_LABEL(DT_NODELABEL(rtc2))) == 0) {
		return true;
	}
#endif

#ifdef CONFIG_COUNTER_TIMER0
	if (strcmp(dev_name, DT_LABEL(DT_NODELABEL(timer0))) == 0) {
		return true;
	}
#endif

#ifdef CONFIG_COUNTER_TIMER1
	if (strcmp(dev_name, DT_LABEL(DT_NODELABEL(timer1))) == 0) {
		return true;
	}
#endif

#ifdef CONFIG_COUNTER_TIMER2
	if (strcmp(dev_name, DT_LABEL(DT_NODELABEL(timer2))) == 0) {
		return true;
	}
#endif

#ifdef CONFIG_COUNTER_TIMER3
	if (strcmp(dev_name, DT_LABEL(DT_NODELABEL(timer3))) == 0) {
		return true;
	}
#endif

#ifdef CONFIG_COUNTER_TIMER4
	if (strcmp(dev_name, DT_LABEL(DT_NODELABEL(timer4))) == 0) {
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

static void test_ds3231_synchronize(void)
{
	const char *dev_name = devices[0];
	const struct device *dev = device_get_binding(dev_name);
	struct sys_notify notify;
	struct k_poll_event evt = K_POLL_EVENT_INITIALIZER(K_POLL_TYPE_SIGNAL,
							   K_POLL_MODE_NOTIFY_ONLY,
							   &sync_sig);
	int rc;
	int res = 0;

	k_poll_signal_reset(&sync_sig);
	sys_notify_init_signal(&notify, &sync_sig);
	rc = maxim_ds3231_synchronize(dev, &notify);
	zassert_true(rc >= 0,
		     "%s: Failed to initiate synchronize: %d", dev_name, rc);

	rc = k_poll(&evt, 1, K_SECONDS(2));
	zassert_true(rc == 0,
		     "%s: Sync wait failed: %d\n", dev_name, rc);

	rc = sys_notify_fetch_result(&notify, &res);

	zassert_true(rc >= 0,
		     "%s: Sync result read failed: %d", dev_name, rc);
	zassert_true(res >= 0,
		     "%s: Sync operation failed: %d", dev_name, res);
}

static void test_ds3231_get_syncpoint(void)
{
	const char *dev_name = devices[0];
	const struct device *dev = device_get_binding(dev_name);
	struct maxim_ds3231_syncpoint syncpoint;
	int rc;

	rc = maxim_ds3231_get_syncpoint(dev, &syncpoint);
	zassert_true(rc >= 0,
		     "%s: Failed to read syncpoint: %d", dev_name, rc);
	zassert_equal(syncpoint.rtc.tv_nsec, 0,
		      "%s: Unexpected nanoseconds\n", dev_name);

	TC_PRINT("Time %u at %u local\n", (uint32_t)syncpoint.rtc.tv_sec,
		 syncpoint.syncclock);
}

static void test_ds3231_req_syncpoint(void)
{
	const char *dev_name = devices[0];
	const struct device *dev = device_get_binding(dev_name);
	struct k_poll_event evt = K_POLL_EVENT_INITIALIZER(K_POLL_TYPE_SIGNAL,
							   K_POLL_MODE_NOTIFY_ONLY,
							   &sync_sig);
	int rc;

	k_poll_signal_reset(&sync_sig);
	rc = maxim_ds3231_req_syncpoint(dev, &sync_sig);
	zassert_true(rc >= 0,
		     "%s: Failed to request syncpoint: %d", dev_name, rc);

	rc = k_poll(&evt, 1, K_SECONDS(2));
	zassert_true(rc == 0,
		     "%s: Syncpoint poll failed: %d\n", dev_name, rc);
	rc = sync_sig.result;
	zassert_true(rc >= 0,
		     "%s: Syncpoint operation failed: %d\n", dev_name, rc);
}

void test_main(void)
{
	const struct device *dev;
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

	k_poll_signal_init(&sync_sig);
	k_object_access_grant(&sync_sig, k_current_get());

	for (i = 0; i < ARRAY_SIZE(devices); i++) {
		dev = device_get_binding(devices[i]);
		zassert_not_null(dev, "Unable to get counter device %s",
				 devices[i]);
		k_object_access_grant(dev, k_current_get());
	}

	ztest_test_suite(test_counter,
			 /* Uses callbacks, run in supervisor mode */
			 ztest_unit_test(test_set_top_value_with_alarm),
			 ztest_unit_test(test_single_shot_alarm_notop),
			 ztest_unit_test(test_single_shot_alarm_top),
			 ztest_unit_test(test_multiple_alarms),
			 ztest_unit_test(test_late_alarm),
			 ztest_unit_test(test_late_alarm_error),
			 ztest_unit_test(test_short_relative_alarm),
			 ztest_unit_test(test_cancelled_alarm_does_not_expire),

			 /* Supervisor-mode driver-specific API */
			 ztest_unit_test(test_ds3231_synchronize),
			 ztest_unit_test(test_ds3231_get_syncpoint),

			 /* User-mode-compatible driver-specific API*/
			 ztest_unit_test(test_ds3231_req_syncpoint),
			 ztest_user_unit_test(test_ds3231_get_syncpoint),

			 /* Supervisor-mode test, takes 63 s so do it last */
			 ztest_unit_test(test_all_channels)
			 );
	ztest_run_test_suite(test_counter);
}
