/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/counter.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/ztest.h>

#if defined(CONFIG_COUNTER_RTC_STM32) && defined(CONFIG_COUNTER_RTC_STM32_SUBSECONDS)

static const struct device *const rtc_dev = DEVICE_DT_GET(DT_INST(0, st_stm32_rtc));
static K_SEM_DEFINE(done, 0, 2);
static atomic_t calls_32;
static atomic_t calls_64;
static bool rearm;
static int rearm_result;
static bool callback_in_isr;
static uint64_t callback_ticks;

static uint32_t alarm_ticks(void)
{
	return MAX(counter_get_frequency(rtc_dev) / 4U, 1U);
}

static void callback_32(const struct device *dev, uint8_t chan, uint32_t ticks, void *user_data)
{
	zassert_equal(dev, rtc_dev);
	zassert_equal(chan, 0);
	zassert_equal(user_data, &done);
	callback_in_isr = k_is_in_isr();
	callback_ticks = ticks;
	atomic_inc(&calls_32);
	k_sem_give(&done);
}

static void callback_64(const struct device *dev, uint8_t chan, uint64_t ticks, void *user_data)
{
	zassert_equal(dev, rtc_dev);
	zassert_equal(chan, 0);
	zassert_equal(user_data, &done);
	callback_in_isr = k_is_in_isr();
	callback_ticks = ticks;
	atomic_inc(&calls_64);
	if (rearm) {
		struct counter_alarm_cfg cfg = {
			.callback = callback_32,
			.ticks = alarm_ticks(),
			.user_data = &done,
		};

		rearm = false;
		rearm_result = counter_set_channel_alarm(dev, 0, &cfg);
	}
	k_sem_give(&done);
}

static void before(void *fixture)
{
	ARG_UNUSED(fixture);
	zassert_true(device_is_ready(rtc_dev));
	zassert_ok(counter_cancel_channel_alarm(rtc_dev, 0));
	zassert_ok(counter_start(rtc_dev));
	k_sem_reset(&done);
	atomic_clear(&calls_32);
	atomic_clear(&calls_64);
	rearm = false;
	rearm_result = -EIO;
	callback_in_isr = false;
}

static void after(void *fixture)
{
	ARG_UNUSED(fixture);
	zassert_ok(counter_cancel_channel_alarm(rtc_dev, 0));
}

ZTEST(stm32_rtc_64, test_callback_width_and_busy)
{
	struct counter_alarm_cfg cfg_32 = {
		.callback = callback_32,
		.ticks = alarm_ticks(),
		.user_data = &done,
	};
	struct counter_alarm_cfg_64 cfg_64 = {
		.callback = callback_64,
		.ticks = alarm_ticks(),
		.user_data = &done,
	};
	uint64_t now;

	zassert_ok(counter_set_channel_alarm(rtc_dev, 0, &cfg_32));
	zassert_equal(counter_set_channel_alarm_64(rtc_dev, 0, &cfg_64), -EBUSY);
	zassert_ok(k_sem_take(&done, K_SECONDS(3)));
	zassert_equal(atomic_get(&calls_32), 1);
	zassert_equal(atomic_get(&calls_64), 0);
	zassert_true(callback_in_isr);

	zassert_ok(counter_set_channel_alarm_64(rtc_dev, 0, &cfg_64));
	zassert_equal(counter_set_channel_alarm(rtc_dev, 0, &cfg_32), -EBUSY);
	zassert_ok(k_sem_take(&done, K_SECONDS(3)));
	zassert_equal(atomic_get(&calls_32), 1);
	zassert_equal(atomic_get(&calls_64), 1);
	zassert_true(callback_in_isr);
	zassert_ok(counter_get_value_64(rtc_dev, &now));
	zassert_true(now >= callback_ticks);
}

ZTEST(stm32_rtc_64, test_cancel_then_change_width)
{
	struct counter_alarm_cfg_64 cfg_64 = {
		.callback = callback_64,
		.ticks = alarm_ticks() * 2U,
		.user_data = &done,
	};
	struct counter_alarm_cfg cfg_32 = {
		.callback = callback_32,
		.ticks = alarm_ticks(),
		.user_data = &done,
	};

	zassert_ok(counter_set_channel_alarm_64(rtc_dev, 0, &cfg_64));
	zassert_ok(counter_cancel_channel_alarm(rtc_dev, 0));
	zassert_ok(counter_set_channel_alarm(rtc_dev, 0, &cfg_32));
	zassert_ok(k_sem_take(&done, K_SECONDS(3)));
	zassert_equal(k_sem_take(&done, K_SECONDS(1)), -EAGAIN);
	zassert_equal(atomic_get(&calls_32), 1);
	zassert_equal(atomic_get(&calls_64), 0);
}

ZTEST(stm32_rtc_64, test_rearm_from_callback)
{
	struct counter_alarm_cfg_64 cfg = {
		.callback = callback_64,
		.ticks = alarm_ticks(),
		.user_data = &done,
	};

	rearm = true;
	zassert_ok(counter_set_channel_alarm_64(rtc_dev, 0, &cfg));
	zassert_ok(k_sem_take(&done, K_SECONDS(3)));
	zassert_ok(rearm_result);
	zassert_ok(k_sem_take(&done, K_SECONDS(3)));
	zassert_equal(atomic_get(&calls_32), 1);
	zassert_equal(atomic_get(&calls_64), 1);
}

ZTEST(stm32_rtc_64, test_range_and_top)
{
	struct counter_alarm_cfg_64 cfg = {
		.callback = callback_64,
		.ticks = (uint64_t)UINT32_MAX + 1U,
		.user_data = &done,
	};
	struct counter_top_cfg_64 top = {
		.ticks = counter_get_top_value_64(rtc_dev),
		.flags = COUNTER_TOP_CFG_DONT_RESET,
	};

	zassert_equal(top.ticks, counter_get_top_value(rtc_dev));
	zassert_equal(counter_set_channel_alarm_64(rtc_dev, 0, &cfg), -EINVAL);
	zassert_ok(counter_set_top_value_64(rtc_dev, &top));
	top.ticks--;
	zassert_equal(counter_set_top_value_64(rtc_dev, &top), -ENOTSUP);
	top.ticks++;
	top.flags = 0;
	zassert_equal(counter_set_top_value_64(rtc_dev, &top), -ENOTSUP);
}

ZTEST_SUITE(stm32_rtc_64, NULL, NULL, before, after, NULL);

#endif
