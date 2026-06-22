/*
 * Copyright 2024 Google LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>
#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/shell/shell.h>
#include <zephyr/ztest.h>

DEVICE_DEFINE(device_0, "device@0", NULL, NULL,
	      NULL, NULL,
	      POST_KERNEL, 0, NULL);

DEVICE_DEFINE(device_1, "device@1", NULL, NULL,
	      NULL, NULL,
	      POST_KERNEL, 1, NULL);

DEVICE_DEFINE(device_2, "xx_device@2", NULL, NULL,
	      NULL, NULL,
	      POST_KERNEL, 2, NULL);

static const struct device *d0 = &DEVICE_NAME_GET(device_0);
static const struct device *d1 = &DEVICE_NAME_GET(device_1);
static const struct device *d2 = &DEVICE_NAME_GET(device_2);

ZTEST(shell_device_filter, test_unfiltered)
{
	zassert_equal_ptr(d0, shell_device_filter(0, NULL));
	zassert_equal_ptr(d1, shell_device_filter(1, NULL));
	zassert_equal_ptr(d2, shell_device_filter(2, NULL));
	zassert_equal_ptr(NULL, shell_device_filter(3, NULL));

	zassert_equal_ptr(d0, shell_device_lookup(0, NULL));
	zassert_equal_ptr(d1, shell_device_lookup(1, NULL));
	zassert_equal_ptr(d2, shell_device_lookup(2, NULL));
	zassert_equal_ptr(NULL, shell_device_lookup(3, NULL));
}

ZTEST(shell_device_filter, test_prefix)
{
	zassert_equal_ptr(d2, shell_device_lookup(0, "xx_"));
	zassert_equal_ptr(NULL, shell_device_lookup(1, "xx_"));
}

static bool pm_device_test_filter(const struct device *dev)
{
	return strstr(dev->name, "@1") != NULL;
}

ZTEST(shell_device_filter, test_filter)
{
	zassert_equal_ptr(d1, shell_device_filter(0, pm_device_test_filter));
	zassert_equal_ptr(NULL, shell_device_filter(1, pm_device_test_filter));
}

ZTEST_SUITE(shell_device_filter, NULL, NULL, NULL, NULL, NULL);
