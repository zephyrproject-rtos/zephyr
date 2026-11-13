/*
 * Copyright (c) 2026 Mario Paja
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Call i2s_get_state() from a user thread on fake I2S devices: one that implements get_state,
 * one that does not, and one the thread has no access to.
 */

#include <zephyr/device.h>
#include <zephyr/drivers/i2s.h>
#include <zephyr/kernel.h>
#include <zephyr/ztest.h>
#include <zephyr/ztest_error_hook.h>

/* Distinct per direction, so the test sees that dir reaches the driver */
static enum i2s_state test_i2s_get_state(const struct device *dev, enum i2s_dir dir)
{
	ARG_UNUSED(dev);

	switch (dir) {
	case I2S_DIR_RX:
		return I2S_STATE_RUNNING;
	case I2S_DIR_TX:
		return I2S_STATE_STOPPING;
	default:
		return I2S_STATE_UNKNOWN;
	}
}

static DEVICE_API(i2s, test_i2s_api) = {
	.get_state = test_i2s_get_state,
};

/* No get_state, so the API falls back to I2S_STATE_UNKNOWN */
static DEVICE_API(i2s, test_i2s_no_state_api) = {
	.get_state = NULL,
};

DEVICE_DEFINE(test_i2s, "test_i2s", NULL, NULL, NULL, NULL, POST_KERNEL,
	      CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &test_i2s_api);
DEVICE_DEFINE(test_i2s_no_state, "test_i2s_no_state", NULL, NULL, NULL, NULL, POST_KERNEL,
	      CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &test_i2s_no_state_api);
DEVICE_DEFINE(test_i2s_no_access, "test_i2s_no_access", NULL, NULL, NULL, NULL, POST_KERNEL,
	      CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &test_i2s_api);

ZTEST_USER(i2s_userspace, test_get_state_dispatch)
{
	const struct device *dev = DEVICE_GET(test_i2s);

	zassert_true(k_is_user_context());
	zassert_equal(I2S_STATE_RUNNING, i2s_get_state(dev, I2S_DIR_RX));
	zassert_equal(I2S_STATE_STOPPING, i2s_get_state(dev, I2S_DIR_TX));
	zassert_equal(I2S_STATE_UNKNOWN, i2s_get_state(dev, I2S_DIR_BOTH));
}

ZTEST_USER(i2s_userspace, test_get_state_not_implemented)
{
	const struct device *dev = DEVICE_GET(test_i2s_no_state);

	zassert_true(k_is_user_context());
	zassert_equal(I2S_STATE_UNKNOWN, i2s_get_state(dev, I2S_DIR_RX));
	zassert_equal(I2S_STATE_UNKNOWN, i2s_get_state(dev, I2S_DIR_TX));
}

ZTEST_USER(i2s_userspace, test_get_state_no_access_oops)
{
	ztest_set_fault_valid(true);
	(void)i2s_get_state(DEVICE_GET(test_i2s_no_access), I2S_DIR_RX);

	ztest_set_fault_valid(false);
	zassert_unreachable("i2s_get_state() accepted a device the thread has no access to");
}

static void *setup(void)
{
	k_object_access_grant(DEVICE_GET(test_i2s), k_current_get());
	k_object_access_grant(DEVICE_GET(test_i2s_no_state), k_current_get());

	return NULL;
}

ZTEST_SUITE(i2s_userspace, NULL, setup, NULL, NULL, NULL);
