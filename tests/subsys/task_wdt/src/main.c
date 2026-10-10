/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-FileCopyrightText: Copyright (c) 2026 Joern Ihlenburg
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Task watchdog with a hardware fallback whose driver sleeps in wdt_setup().
 *
 * The MCUX wdog32 driver waits up to 20 ms in wdt_setup() for the
 * peripheral to take its configuration. task_wdt_add() used to start the
 * hardware watchdog while holding channels_lock, with the new channel's
 * reload period set and its deadline still at K_TICKS_FOREVER. On a single
 * core the sleep runs another thread; when that thread adds or feeds a
 * channel, schedule_next_timeout() takes the half-registered channel's
 * deadline of minus one as the earliest and fires the timer at once, so the
 * channel expires right after registration.
 */

#include <zephyr/device.h>
#include <zephyr/drivers/watchdog.h>
#include <zephyr/kernel.h>
#include <zephyr/task_wdt/task_wdt.h>
#include <zephyr/ztest.h>

#define FAKE_SETUP_SLEEP_MS 20
#define RELOAD_MS           500
#define OBSERVE_MS          200

static atomic_t setup_calls;
static atomic_t feed_calls;

static int fake_setup(const struct device *dev, uint8_t options)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(options);

	atomic_inc(&setup_calls);
	/* Like the MCUX wdog32 driver: the configuration takes a while. */
	k_msleep(FAKE_SETUP_SLEEP_MS);

	return 0;
}

static int fake_disable(const struct device *dev)
{
	ARG_UNUSED(dev);

	return 0;
}

static int fake_install_timeout(const struct device *dev, const struct wdt_timeout_cfg *cfg)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(cfg);

	return 0;
}

static int fake_feed(const struct device *dev, int channel_id)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(channel_id);

	atomic_inc(&feed_calls);

	return 0;
}

static DEVICE_API(wdt, fake_wdt_api) = {
	.setup = fake_setup,
	.disable = fake_disable,
	.install_timeout = fake_install_timeout,
	.feed = fake_feed,
};

DEVICE_DEFINE(fake_wdt, "FAKE_WDT", NULL, NULL, NULL, NULL, POST_KERNEL,
	      CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &fake_wdt_api);

/* Set by the channel callback: the id of the channel that expired, or -1. */
static atomic_t expired_channel = ATOMIC_INIT(-1);

static void on_expire(int channel_id, void *user_data)
{
	ARG_UNUSED(user_data);

	atomic_set(&expired_channel, channel_id);
}

static K_THREAD_STACK_DEFINE(second_stack, 1024);
static struct k_thread second_thread;
static int second_channel = -1;

static void second_adder(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	second_channel = task_wdt_add(RELOAD_MS, on_expire, NULL);
}

static void *suite_setup(void)
{
	zassert_ok(task_wdt_init(DEVICE_GET(fake_wdt)));

	return NULL;
}

/* A failed test must not leave a channel behind that expires into the next. */
static void test_after(void *fixture)
{
	ARG_UNUSED(fixture);

	for (int id = 0; id < CONFIG_TASK_WDT_CHANNELS; id++) {
		(void)task_wdt_delete(id);
	}
}

/*
 * Two threads register a channel each while the hardware watchdog is being
 * started. Neither channel may expire, and the hardware is set up once.
 */
ZTEST(task_wdt_hw_fallback, test_concurrent_add_does_not_expire)
{
	int first_channel;

	atomic_set(&expired_channel, -1);
	atomic_set(&setup_calls, 0);
	second_channel = -1;

	/* The second thread runs as soon as this one sleeps in wdt_setup(). */
	k_thread_create(&second_thread, second_stack, K_THREAD_STACK_SIZEOF(second_stack),
			second_adder, NULL, NULL, NULL,
			k_thread_priority_get(k_current_get()), 0, K_MSEC(FAKE_SETUP_SLEEP_MS / 4));

	first_channel = task_wdt_add(RELOAD_MS, on_expire, NULL);
	zassert_true(first_channel >= 0, "first add failed: %d", first_channel);

	zassert_ok(k_thread_join(&second_thread, K_MSEC(10 * FAKE_SETUP_SLEEP_MS)));
	zassert_true(second_channel >= 0, "second add failed: %d", second_channel);
	zassert_not_equal(first_channel, second_channel);

	k_msleep(OBSERVE_MS);

	zassert_equal(atomic_get(&expired_channel), -1,
		      "channel %ld expired %d ms after registration",
		      atomic_get(&expired_channel), OBSERVE_MS);
	zassert_equal(atomic_get(&setup_calls), 1, "wdt_setup() called %ld times",
		      atomic_get(&setup_calls));

	zassert_ok(task_wdt_delete(first_channel));
	zassert_ok(task_wdt_delete(second_channel));
}

/* A channel that is not fed expires with its own id, and only that one. */
ZTEST(task_wdt_hw_fallback, test_unfed_channel_expires)
{
	int channel;

	atomic_set(&expired_channel, -1);

	channel = task_wdt_add(RELOAD_MS, on_expire, NULL);
	zassert_true(channel >= 0, "add failed: %d", channel);

	k_msleep(RELOAD_MS / 2);
	zassert_equal(atomic_get(&expired_channel), -1, "expired early");

	k_msleep(RELOAD_MS);
	zassert_equal(atomic_get(&expired_channel), channel, "expected channel %d, got %ld",
		      channel, atomic_get(&expired_channel));

	zassert_ok(task_wdt_delete(channel));
}

/* The background channel keeps feeding the hardware fallback. */
ZTEST(task_wdt_hw_fallback, test_hardware_is_fed)
{
	atomic_val_t before = atomic_get(&feed_calls);

	k_msleep(3 * CONFIG_TASK_WDT_MIN_TIMEOUT);

	zassert_true(atomic_get(&feed_calls) > before, "hardware watchdog not fed");
}

ZTEST_SUITE(task_wdt_hw_fallback, NULL, suite_setup, NULL, test_after, NULL);
