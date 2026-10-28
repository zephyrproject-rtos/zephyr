/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Linumiz
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>

#include <zephyr/device.h>
#include <zephyr/drivers/rc.h>
#include <zephyr/drivers/rc/rc_loopback.h>
#include <zephyr/kernel.h>
#include <zephyr/rc/rc.h>
#include <zephyr/ztest.h>

#define TEST_PROTO_EVT 0x1fa
#define KEYUP_MS       6
#define LOG_SIZE       8

static const struct device *const dev7 = DEVICE_DT_GET(DT_NODELABEL(rc_loopback7));
static const struct device *const dev8 = DEVICE_DT_GET(DT_NODELABEL(rc_loopback8));

static int marker;

static struct rc_event dev8_log[LOG_SIZE];
static size_t dev8_count;
static bool dev8_on_rc_rx;
static void *dev8_user_data;
static int dev8_sync_ret;
static atomic_t all_dev8_count;
static atomic_t dev7_count;
static K_SEM_DEFINE(dev8_sem, 0, LOG_SIZE);

static void dev8_cb(const struct device *dev, const struct rc_event *evt, void *user_data)
{
	struct rc_event unused;

	zassert_equal(dev, dev8);
	if (dev8_count < LOG_SIZE) {
		dev8_log[dev8_count++] = *evt;
	}
	dev8_on_rc_rx = (strcmp(k_thread_name_get(k_current_get()), "rc_rx") == 0);
	dev8_user_data = user_data;
	dev8_sync_ret = rc_receive_sync(dev, &unused, K_NO_WAIT);
	k_sem_give(&dev8_sem);
}

static void all_cb(const struct device *dev, const struct rc_event *evt, void *user_data)
{
	ARG_UNUSED(evt);
	ARG_UNUSED(user_data);

	if (dev == dev8) {
		atomic_inc(&all_dev8_count);
	}
}

static void dev7_cb(const struct device *dev, const struct rc_event *evt, void *user_data)
{
	ARG_UNUSED(evt);
	ARG_UNUSED(user_data);

	zassert_equal(dev, dev7);
	atomic_inc(&dev7_count);
}

RC_CALLBACK_DEFINE(DEVICE_DT_GET(DT_NODELABEL(rc_loopback8)), dev8_cb, &marker);
RC_CALLBACK_DEFINE(NULL, all_cb, NULL);
RC_CALLBACK_DEFINE(DEVICE_DT_GET(DT_NODELABEL(rc_loopback7)), dev7_cb, NULL);

static void inject(uint32_t scancode)
{
	const uint32_t word = RC_RAW_PULSE_US(scancode);

	zassert_ok(rc_loopback_inject(dev8, &word, 1));
}

static void rc_callback_before(void *fixture)
{
	ARG_UNUSED(fixture);

	k_sleep(K_MSEC(4 * KEYUP_MS));
	k_sem_reset(&dev8_sem);
	dev8_count = 0;
	dev8_sync_ret = 0;
	atomic_clear(&all_dev8_count);
	atomic_clear(&dev7_count);
	zassert_ok(rc_rx_enable(dev8, true));
}

static void rc_callback_after(void *fixture)
{
	ARG_UNUSED(fixture);

	zassert_ok(rc_rx_enable(dev8, false));
}

ZTEST(rc_callback, test_event_and_release)
{
	inject(100);
	zassert_ok(k_sem_take(&dev8_sem, K_MSEC(500)));
	zassert_ok(k_sem_take(&dev8_sem, K_MSEC(500)));

	zassert_equal(dev8_count, 2U);
	zassert_equal(dev8_log[0].proto, TEST_PROTO_EVT);
	zassert_equal(dev8_log[0].scancode, 100U);
	zassert_equal(dev8_log[0].flags, 0);
	zassert_equal(dev8_log[1].scancode, 100U);
	zassert_equal(dev8_log[1].flags, RC_EVENT_FLAG_RELEASE);
	zassert_equal(dev8_user_data, &marker);
	zassert_true(dev8_on_rc_rx, "callback not on rc_rx");
}

ZTEST(rc_callback, test_receive_sync_deadlock)
{
	inject(100);
	zassert_ok(k_sem_take(&dev8_sem, K_MSEC(500)));
	zassert_equal(dev8_sync_ret, -EDEADLK);
}

ZTEST(rc_callback, test_filter)
{
	inject(100);
	zassert_ok(k_sem_take(&dev8_sem, K_MSEC(500)));
	zassert_ok(k_sem_take(&dev8_sem, K_MSEC(500)));

	zassert_equal(atomic_get(&all_dev8_count), 2);
	zassert_equal(atomic_get(&dev7_count), 0);
}

ZTEST_SUITE(rc_callback, NULL, NULL, rc_callback_before, rc_callback_after, NULL);
