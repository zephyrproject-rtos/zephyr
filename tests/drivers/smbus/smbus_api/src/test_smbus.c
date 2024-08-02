/*
 * Copyright (c) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * Verify SMBUS Basic API, tests should work on any board
 */

#include <zephyr/kernel.h>
#include <zephyr/ztest.h>
#include <zephyr/drivers/smbus.h>

#include <smbus_utils.h>

BUILD_ASSERT(DT_NODE_HAS_STATUS(DT_NODELABEL(smbus0), okay),
	     "SMBus node is disabled!");

#define FAKE_ADDRESS	0x10

/**
 * The test is run in userspace only if CONFIG_USERSPACE option is
 * enabled, otherwise it is the same as ZTEST()
 */
ZTEST_USER(test_smbus_general, test_smbus_basic_api)
{
	const struct device *const dev = DEVICE_DT_GET(DT_NODELABEL(smbus0));
	uint32_t cfg = SMBUS_MODE_CONTROLLER;
	uint32_t cfg_tmp;
	int ret;

	zassert_true(device_is_ready(dev), "Device is not ready");

	ret = smbus_configure(dev, cfg);
	zassert_ok(ret, "SMBUS config failed");

	ret = smbus_get_config(dev, &cfg_tmp);
	zassert_ok(ret, "SMBUS get_config failed");

	zassert_equal(cfg, cfg_tmp, "get_config returned invalid config");
}

/**
 * The test is run in userspace only if CONFIG_USERSPACE option is
 * enabled, otherwise it is the same as ZTEST()
 */
ZTEST(test_smbus_general, test_smbus_smbalert_api)
{
	const struct device *const dev = DEVICE_DT_GET(DT_NODELABEL(smbus0));
	void *dummy; /* For the dummy function pointer use this */
	int ret;

	/* Note! Only for test using stack variables to ease userspace tests */
	struct smbus_callback callback = {
		.handler = (void *)&dummy,
		.addr = FAKE_ADDRESS,
	};

	zassert_true(device_is_ready(dev), "Device is not ready");

	/* Smbalert callbacks */

	/* Try to remove not existing callback */
	ret = smbus_smbalert_remove_cb(dev, &callback);
	if (IS_ENABLED(CONFIG_SMBUS_INTEL_PCH_SMBALERT)) {
		zassert_equal(ret, -ENOENT, "Callback remove failed");
	} else {
		zassert_equal(ret, -ENOSYS, "Check for ENOSYS failed");
	}

	/* Set callback */
	ret = smbus_smbalert_set_cb(dev, &callback);
	if (IS_ENABLED(CONFIG_SMBUS_INTEL_PCH_SMBALERT)) {
		zassert_ok(ret, "Callback set failed");
	} else {
		zassert_equal(ret, -ENOSYS, "Check for ENOSYS failed");
	}

	/* Remove existing callback */
	ret = smbus_smbalert_remove_cb(dev, &callback);
	if (IS_ENABLED(CONFIG_SMBUS_INTEL_PCH_SMBALERT)) {
		zassert_ok(ret, "Callback remove failed");
	} else {
		zassert_equal(ret, -ENOSYS, "Check for ENOSYS failed");
	}
}

/**
 * The test is run in userspace only if CONFIG_USERSPACE option is
 * enabled, otherwise it is the same as ZTEST()
 */
ZTEST(test_smbus_general, test_smbus_host_notify_api)
{
	const struct device *const dev = DEVICE_DT_GET(DT_NODELABEL(smbus0));
	void *dummy; /* For the dummy function pointer use this */
	int ret;

	/* Note! Only for test using stack variables to ease userspace tests */
	struct smbus_callback callback = {
		.handler = (void *)&dummy,
		.addr = FAKE_ADDRESS,
	};

	zassert_true(device_is_ready(dev), "Device is not ready");

	/* Host Notify callbacks */

	/* Try to remove not existing callback */
	ret = smbus_host_notify_remove_cb(dev, &callback);
	if (IS_ENABLED(CONFIG_SMBUS_INTEL_PCH_HOST_NOTIFY)) {
		zassert_equal(ret, -ENOENT, "Callback remove failed");
	} else {
		zassert_equal(ret, -ENOSYS, "Check for ENOSYS failed");
	}

	/* Set callback */
	ret = smbus_host_notify_set_cb(dev, &callback);
	if (IS_ENABLED(CONFIG_SMBUS_INTEL_PCH_HOST_NOTIFY)) {
		zassert_ok(ret, "Callback set failed");
	} else {
		zassert_equal(ret, -ENOSYS, "Check for ENOSYS failed");
	}

	/* Remove existing callback */
	ret = smbus_host_notify_remove_cb(dev, &callback);
	if (IS_ENABLED(CONFIG_SMBUS_INTEL_PCH_HOST_NOTIFY)) {
		zassert_ok(ret, "Callback remove failed");
	} else {
		zassert_equal(ret, -ENOSYS, "Check for ENOSYS failed");
	}
}

/**
 * The test is run in userspace only if CONFIG_USERSPACE option is
 * enabled, otherwise it is the same as ZTEST()
 */
ZTEST_USER(test_smbus_general, test_smbus_api_errors)
{
	const struct device *const dev = DEVICE_DT_GET(DT_NODELABEL(smbus0));
	uint8_t fake_addr = 0x10;
	uint8_t buf[2];
	int ret;

	zassert_true(device_is_ready(dev), "Device is not ready");

	/* Test parsing SMBus quick */
	ret = smbus_quick(dev, fake_addr, 3);
	zassert_equal(ret, -EINVAL, "Wrong parameter check failed");

	/* Test parsing SMBus block_write */
	ret = smbus_block_write(dev, fake_addr, 0, 0, buf);
	zassert_equal(ret, -EINVAL, "Wrong parameter check failed");
	ret = smbus_block_write(dev, fake_addr, 0, SMBUS_BLOCK_BYTES_MAX + 1,
				buf);
	zassert_equal(ret, -EINVAL, "Wrong parameter check failed");
}

/* Setup is needed for userspace access */
static void *smbus_test_setup(void)
{
	const struct device *const dev = DEVICE_DT_GET(DT_NODELABEL(smbus0));

	zassert_true(device_is_ready(dev), "Device is not ready");

	k_object_access_grant(dev, k_current_get());

	return NULL;
}

ZTEST_SUITE(test_smbus_general, NULL, smbus_test_setup, NULL, NULL, NULL);
