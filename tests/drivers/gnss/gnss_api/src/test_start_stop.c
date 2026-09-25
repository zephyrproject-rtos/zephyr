/*
 * Copyright 2026 CampOS
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/gnss.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/ztest.h>

#define TEST_FIX_INTERVAL_MS    200
#define TEST_VALIDATE_PERIOD    K_MSEC(2500)

static const struct device *dev = DEVICE_DT_GET(DT_ALIAS(gnss));

static atomic_t callback_count_atom = ATOMIC_INIT(0);

static void gnss_data_cb(const struct device *dev, const struct gnss_data *data)
{
	atomic_inc(&callback_count_atom);
}

GNSS_DATA_CALLBACK_DEFINE(DEVICE_DT_GET(DT_ALIAS(gnss)), gnss_data_cb);

ZTEST(gnss_api, test_start_stop)
{
	int ret;
	uint32_t callback_count;

	ret = gnss_set_fix_rate(dev, TEST_FIX_INTERVAL_MS);
	zassert_true(ret == 0 || ret == -ENOSYS, "failed to set fix rate");

	ret = gnss_stop(dev);
	if (ret == -ENOSYS) {
		ztest_test_skip();
	}
	zassert_ok(ret, "failed to stop GNSS");

	atomic_set(&callback_count_atom, 0);
	k_sleep(TEST_VALIDATE_PERIOD);
	callback_count = atomic_get(&callback_count_atom);
	zassert_equal(callback_count, 0, "received %u GNSS updates while stopped",
		     callback_count);

	ret = gnss_start(dev, GNSS_HOT_START);
	zassert_ok(ret, "failed to start GNSS");

	atomic_set(&callback_count_atom, 0);
	k_sleep(TEST_VALIDATE_PERIOD);
	callback_count = atomic_get(&callback_count_atom);
	zassert_true(callback_count > 0, "received no GNSS updates after start");
}
