/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/counter.h>
#include <zephyr/irq.h>
#include <zephyr/kernel.h>
#include <zephyr/ztest.h>

#if defined(CONFIG_SOC_SERIES_STM32N6X) && defined(CONFIG_COUNTER_RTC_STM32_SUBSECONDS)

static const struct device *const rtc_dev = DEVICE_DT_GET(DT_INST(0, st_stm32_rtc));
static K_SEM_DEFINE(done, 0, 1);
static bool callback_in_isr;

static void callback(const struct device *dev, uint8_t chan, uint64_t ticks, void *user_data)
{
	ARG_UNUSED(ticks);
	zassert_equal(dev, rtc_dev);
	zassert_equal(chan, 0);
	zassert_equal(user_data, &done);
	callback_in_isr = k_is_in_isr();
	k_sem_give(&done);
}

static void before(void *fixture)
{
	ARG_UNUSED(fixture);
	zassert_true(device_is_ready(rtc_dev));
	zassert_ok(counter_cancel_channel_alarm(rtc_dev, 0));
	zassert_ok(counter_start(rtc_dev));
	zassert_ok(counter_set_guard_period_64(rtc_dev, counter_get_frequency(rtc_dev),
					     COUNTER_GUARD_PERIOD_LATE_TO_SET));
	k_sem_reset(&done);
	callback_in_isr = false;
}

static void after(void *fixture)
{
	ARG_UNUSED(fixture);
	zassert_ok(counter_cancel_channel_alarm(rtc_dev, 0));
	zassert_ok(counter_set_guard_period_64(rtc_dev, 0, COUNTER_GUARD_PERIOD_LATE_TO_SET));
}

ZTEST(stm32n6_rtc_wakeup, test_absolute_future)
{
	uint32_t now;
	struct counter_alarm_cfg_64 cfg = {
		.callback = callback,
		.user_data = &done,
		.flags = COUNTER_ALARM_CFG_ABSOLUTE,
	};

	zassert_ok(counter_get_value(rtc_dev, &now));
	cfg.ticks = (uint32_t)(now + counter_get_frequency(rtc_dev) / 4U);
	zassert_ok(counter_set_channel_alarm_64(rtc_dev, 0, &cfg));
	zassert_ok(k_sem_take(&done, K_SECONDS(3)));
	zassert_true(callback_in_isr);
}

ZTEST(stm32n6_rtc_wakeup, test_late_expiry)
{
	uint32_t now;
	struct counter_alarm_cfg_64 cfg = {
		.callback = callback,
		.user_data = &done,
		.flags = COUNTER_ALARM_CFG_ABSOLUTE,
	};

	zassert_ok(counter_get_value(rtc_dev, &now));
	cfg.ticks = (uint32_t)(now - 1U);
	zassert_equal(counter_set_channel_alarm_64(rtc_dev, 0, &cfg), -ETIME);
	zassert_equal(k_sem_take(&done, K_MSEC(20)), -EAGAIN);
	cfg.flags |= COUNTER_ALARM_CFG_EXPIRE_WHEN_LATE;
	zassert_equal(counter_set_channel_alarm_64(rtc_dev, 0, &cfg), -ETIME);
	zassert_ok(k_sem_take(&done, K_SECONDS(1)));
	zassert_true(callback_in_isr);
}

ZTEST(stm32n6_rtc_wakeup, test_cancel_pending_late_expiry)
{
	uint32_t now;
	unsigned int key;
	int alarm_ret;
	int cancel_ret;
	struct counter_alarm_cfg_64 cfg = {
		.callback = callback,
		.user_data = &done,
		.flags = COUNTER_ALARM_CFG_ABSOLUTE | COUNTER_ALARM_CFG_EXPIRE_WHEN_LATE,
	};

	zassert_ok(counter_get_value(rtc_dev, &now));
	cfg.ticks = (uint32_t)(now - 1U);
	key = irq_lock();
	alarm_ret = counter_set_channel_alarm_64(rtc_dev, 0, &cfg);
	cancel_ret = counter_cancel_channel_alarm(rtc_dev, 0);
	irq_unlock(key);
	zassert_equal(alarm_ret, -ETIME);
	zassert_ok(cancel_ret);
	zassert_equal(k_sem_take(&done, K_MSEC(20)), -EAGAIN);
}

ZTEST(stm32n6_rtc_wakeup, test_guard_range)
{
	uint64_t guard = counter_get_guard_period_64(rtc_dev, COUNTER_GUARD_PERIOD_LATE_TO_SET);

	zassert_equal(counter_set_guard_period_64(rtc_dev, (uint64_t)UINT32_MAX + 1U,
						COUNTER_GUARD_PERIOD_LATE_TO_SET), -EINVAL);
	zassert_equal(counter_get_guard_period_64(rtc_dev,
					 COUNTER_GUARD_PERIOD_LATE_TO_SET), guard);
	zassert_equal(counter_get_guard_period(rtc_dev, COUNTER_GUARD_PERIOD_LATE_TO_SET), guard);
}

ZTEST(stm32n6_rtc_wakeup, test_short_relative_alarm)
{
	struct counter_alarm_cfg_64 cfg = {
		.callback = callback,
		.user_data = &done,
		.ticks = 1U,
	};
	uint32_t tick_us = (uint32_t)counter_ticks_to_us(rtc_dev, 1U);

	/* Keep the generic test's three-tick deadline; do not use its capability skip. */
	for (uint32_t i = 0U; i < 100U; i++) {
		zassert_ok(counter_set_channel_alarm_64(rtc_dev, 0, &cfg));
		k_busy_wait(3U * tick_us);
		zassert_ok(k_sem_take(&done, K_NO_WAIT));
		zassert_true(callback_in_isr);
	}
}

ZTEST(stm32n6_rtc_wakeup, test_absolute_target_during_rearm)
{
	struct counter_alarm_cfg_64 cfg = {
		.callback = callback,
		.user_data = &done,
	};
	uint32_t late_count;
	uint32_t now;

	/* Exercise setup crossing separately from an alarm already in the guard zone. */
	zassert_ok(counter_set_guard_period_64(rtc_dev, 0U, COUNTER_GUARD_PERIOD_LATE_TO_SET));
	for (uint32_t expire = 0U; expire < 2U; expire++) {
		late_count = 0U;
		for (uint32_t i = 0U; i < 100U; i++) {
			int ret;

			cfg.flags = 0U;
			cfg.ticks = 40U * counter_get_frequency(rtc_dev);
			zassert_ok(counter_set_channel_alarm_64(rtc_dev, 0, &cfg));
			k_busy_wait(5000U + i);
			zassert_ok(counter_cancel_channel_alarm(rtc_dev, 0));
			zassert_ok(counter_get_value(rtc_dev, &now));
			cfg.ticks = (uint32_t)(now + 1U);
			cfg.flags = COUNTER_ALARM_CFG_ABSOLUTE |
				(expire != 0U ? COUNTER_ALARM_CFG_EXPIRE_WHEN_LATE : 0U);
			ret = counter_set_channel_alarm_64(rtc_dev, 0, &cfg);
			zassert_true(ret == 0 || ret == -ETIME, "Unexpected alarm error: %d", ret);
			if (ret == -ETIME) {
				late_count++;
				if (expire != 0U) {
					zassert_ok(k_sem_take(&done, K_SECONDS(1)));
					zassert_true(callback_in_isr);
				} else {
					zassert_equal(k_sem_take(&done, K_MSEC(20)), -EAGAIN);
				}
			} else {
				zassert_ok(k_sem_take(&done, K_SECONDS(1)));
			}
			zassert_ok(counter_cancel_channel_alarm(rtc_dev, 0));
		}
		zassert_true(late_count > 0U, "No target crossed during rearm; case not exercised");
	}
}

ZTEST_SUITE(stm32n6_rtc_wakeup, NULL, NULL, before, after, NULL);

#endif

#if defined(CONFIG_SOC_SERIES_STM32N6X) && defined(CONFIG_COUNTER_RTC_STM32) && \
	!defined(CONFIG_COUNTER_RTC_STM32_SUBSECONDS)

static const struct device *const rtc_dev = DEVICE_DT_GET(DT_INST(0, st_stm32_rtc));
static K_SEM_DEFINE(done, 0, 1);

static void callback(const struct device *dev, uint8_t chan, uint32_t ticks, void *user_data)
{
	ARG_UNUSED(ticks);
	zassert_equal(dev, rtc_dev);
	zassert_equal(chan, 0);
	zassert_equal(user_data, &done);
	k_sem_give(&done);
}

ZTEST(stm32n6_rtc_wakeup, test_ckspre_cancel_rearm)
{
	struct counter_alarm_cfg cfg = {
		.callback = callback,
		.user_data = &done,
		/* Beyond the DIV16 range: select CKSPRE without subseconds. */
		.ticks = 40U,
	};

	zassert_true(device_is_ready(rtc_dev));
	zassert_ok(counter_start(rtc_dev));
	for (uint32_t i = 0U; i < 32U; i++) {
		zassert_ok(counter_set_channel_alarm(rtc_dev, 0, &cfg));
		/* Allow enable synchronization before cancelling and immediately rearming. */
		k_busy_wait(5000U);
		zassert_ok(counter_cancel_channel_alarm(rtc_dev, 0));
		zassert_equal(k_sem_take(&done, K_NO_WAIT), -EBUSY);
	}
	/* Finish with a short alarm to verify ownership was released. */
	cfg.ticks = 1U;
	zassert_ok(counter_set_channel_alarm(rtc_dev, 0, &cfg));
	zassert_ok(k_sem_take(&done, K_SECONDS(3)));
	zassert_ok(counter_cancel_channel_alarm(rtc_dev, 0));
}

ZTEST_SUITE(stm32n6_rtc_wakeup, NULL, NULL, NULL, NULL, NULL);

#endif
