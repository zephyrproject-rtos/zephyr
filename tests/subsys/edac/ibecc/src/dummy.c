/*
 * Copyright (c) 2021 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/zephyr.h>
#include <zephyr/device.h>
#include <ztest.h>

#include <zephyr/drivers/edac.h>

/**
 * EDAC dummy is used for coverage tests for -ENOSYS returns
 */

int edac_dummy_init(const struct device *dev)
{
	return 0;
}

static const struct edac_driver_api edac_dummy_api = { 0 };

DEVICE_DEFINE(dummy_edac, "dummy_edac", edac_dummy_init, NULL,
	      NULL, NULL,
	      POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
	      &edac_dummy_api);

void test_edac_dummy_api(void)
{
	const struct device *dev = device_get_binding("dummy_edac");
	uint64_t value;
	int ret;

	zassert_not_null(dev, "Device not found");

	/* Error log API */

	ret = edac_ecc_error_log_get(dev, &value);
	zassert_equal(ret, -ENOSYS, "dummy api failed");

	ret = edac_ecc_error_log_clear(dev);
	zassert_equal(ret, -ENOSYS, "dummy api failed");

	ret = edac_parity_error_log_get(dev, &value);
	zassert_equal(ret, -ENOSYS, "dummy api failed");

	ret = edac_parity_error_log_clear(dev);
	zassert_equal(ret, -ENOSYS, "dummy api failed");

	/* Error stat API */

	ret = edac_errors_cor_get(dev);
	zassert_equal(ret, -ENOSYS, "dummy api failed");

	ret = edac_errors_uc_get(dev);
	zassert_equal(ret, -ENOSYS, "dummy api failed");

	/* Notification API */

	/* It is OK to use NULL as a callback pointer */
	ret = edac_notify_callback_set(dev, NULL);
	zassert_equal(ret, -ENOSYS, "dummy api failed");

	/* Injection API */

	ret = edac_inject_set_param1(dev, 0x0);
	zassert_equal(ret, -ENOSYS, "dummy api failed");

	ret = edac_inject_get_param1(dev, &value);
	zassert_equal(ret, -ENOSYS, "dummy api failed");

	ret = edac_inject_set_param2(dev, 0x0);
	zassert_equal(ret, -ENOSYS, "dummy api failed");

	ret = edac_inject_get_param2(dev, &value);
	zassert_equal(ret, -ENOSYS, "dummy api failed");

	ret = edac_inject_set_error_type(dev, 0x0);
	zassert_equal(ret, -ENOSYS, "dummy api failed");

	ret = edac_inject_get_error_type(dev, (uint32_t *)&value);
	zassert_equal(ret, -ENOSYS, "dummy api failed");

	ret = edac_inject_error_trigger(dev);
	zassert_equal(ret, -ENOSYS, "dummy api failed");
}
