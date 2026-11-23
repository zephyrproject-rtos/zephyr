/*
 * Copyright 2024 Trackunit Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/gnss.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/ztest.h>

static const struct device *dev = DEVICE_DT_GET(DT_ALIAS(gnss));
static const enum gnss_navigation_mode nav_modes[] = {
	GNSS_NAVIGATION_MODE_ZERO_DYNAMICS,
	GNSS_NAVIGATION_MODE_LOW_DYNAMICS,
	GNSS_NAVIGATION_MODE_BALANCED_DYNAMICS,
	GNSS_NAVIGATION_MODE_HIGH_DYNAMICS,
};

static bool test_set_nav_mode(enum gnss_navigation_mode nav_mode)
{
	int ret;

	ret = gnss_set_navigation_mode(dev, nav_mode);
	if (ret == -ENOSYS) {
		ztest_test_skip();
	}
	if (ret == -EINVAL) {
		return false;
	}
	zassert_ok(ret, "failed to set navigation mode %u", nav_mode);
	return true;
}

static void test_validate_nav_mode(enum gnss_navigation_mode nav_mode)
{
	int ret;
	enum gnss_navigation_mode set_nav_mode;

	ret = gnss_get_navigation_mode(dev, &set_nav_mode);
	if (ret == -ENOSYS) {
		return;
	}
	zassert_ok(ret, "failed to get navigation mode %u", nav_mode);
}

ZTEST(gnss_api, test_navigation_mode)
{
	enum gnss_navigation_mode nav_mode;

	for (uint8_t i = 0; i < ARRAY_SIZE(nav_modes); i++) {
		nav_mode = nav_modes[i];

		if (!test_set_nav_mode(nav_mode)) {
			continue;
		}

		test_validate_nav_mode(nav_mode);
	}
}
