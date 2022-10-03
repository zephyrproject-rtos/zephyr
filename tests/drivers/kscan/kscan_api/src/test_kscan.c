/*
 * Copyright (c) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <stdlib.h>
#include <zephyr/drivers/kscan.h>
#include <zephyr/kernel.h>
#include <zephyr/ztest.h>

static void kb_callback(const struct device *dev, uint32_t row, uint32_t col,
			bool pressed)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(row);
	ARG_UNUSED(col);
	ARG_UNUSED(pressed);
}

static int test_kb_callback(void)
{
	const struct device *const kscan_dev = DEVICE_DT_GET(DT_ALIAS(kscan0));

	if (!device_is_ready(kscan_dev)) {
		TC_PRINT("KBSCAN device is not ready\n");
		return TC_FAIL;
	}

	if (kscan_config(kscan_dev, kb_callback) != 0) {
		TC_PRINT("Unexpected error code received\n");
		return TC_FAIL;
	}

	return TC_PASS;
}

static int test_null_callback(void)
{
	const struct device *const kscan_dev = DEVICE_DT_GET(DT_ALIAS(kscan0));

	if (!device_is_ready(kscan_dev)) {
		TC_PRINT("KBSCAN device is not ready\n");
		return TC_FAIL;
	}

	if (kscan_config(kscan_dev, NULL) != -EINVAL) {
		TC_PRINT("Unexpected error code received\n");
		return TC_FAIL;
	}

	return TC_PASS;
}

static int test_disable_enable_callback(void)
{
	const struct device *const kscan_dev = DEVICE_DT_GET(DT_ALIAS(kscan0));

	if (!device_is_ready(kscan_dev)) {
		TC_PRINT("KBSCAN device is not ready\n");
		return TC_FAIL;
	}

	if (kscan_config(kscan_dev, kb_callback) != 0) {
		TC_PRINT("Unexpected error code received\n");
		return TC_FAIL;
	}

	if (kscan_disable_callback(kscan_dev) != 0) {
		TC_PRINT("Error while disabling callback\n");
		return TC_FAIL;
	}

	if (kscan_enable_callback(kscan_dev) != 0) {
		TC_PRINT("Error while enabling callback\n");
		return TC_FAIL;
	}

	return TC_PASS;
}

ZTEST(kscan_basic, test_init_callback)
{
	/* Configure kscan matrix with an appropriate callback */
	zassert_true(test_kb_callback() == TC_PASS);
	k_sleep(K_MSEC(1000));

	/* Configure kscan with a null callback */
	zassert_true(test_null_callback() == TC_PASS);
}

ZTEST(kscan_basic, test_control_callback)
{
	/* Disable/enable notifications to user */
	zassert_true(test_disable_enable_callback() == TC_PASS);
	k_sleep(K_MSEC(1000));
}
