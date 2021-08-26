/*
 * Copyright (c) 2021 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <ztest.h>
#include <device.h>
#include <pm/pm.h>

#define TEST_I2C DT_NODELABEL(test_i2c)
#define TEST_DEVA DT_NODELABEL(test_dev_a)
#define TEST_DEVB DT_NODELABEL(test_dev_b)


static int dev_init(const struct device *dev)
{
	ARG_UNUSED(dev);

	return 0;
}

/* Device don't need to do anything here. What we are testing
 * is the subsystem
 */
static int dev_pm_control(const struct device *dev,
		    enum pm_device_action action)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(action);

	return 0;
}

DEVICE_DT_DEFINE(TEST_I2C, dev_init, dev_pm_control,
		 NULL, NULL, POST_KERNEL, 10, NULL);
DEVICE_DT_DEFINE(TEST_DEVA, dev_init, dev_pm_control,
		 NULL, NULL, POST_KERNEL, 20, NULL);
DEVICE_DT_DEFINE(TEST_DEVB, dev_init, dev_pm_control,
		 NULL, NULL, POST_KERNEL, 20, NULL);

static const struct device *dev_i2c, *dev_a, *dev_b;

static void test_parent_suspend(void)
{
	int ret;

	dev_i2c = DEVICE_DT_GET(TEST_I2C);
	zassert_not_null(dev_i2c, "Failed to get device");

	dev_a = DEVICE_DT_GET(TEST_DEVA);
	zassert_not_null(dev_a, "Failed to get device");

	dev_b = DEVICE_DT_GET(TEST_DEVB);
	zassert_not_null(dev_b, "Failed to get device");

	ret = pm_device_state_set(dev_i2c, PM_DEVICE_STATE_SUSPENDED);
	/* dev_i2c shall not be suspended because there are active
	 * devices depending on it.
	 */
	zassert_not_equal(ret, 0, "Device I2C should not be suspended");

	ret = pm_device_state_set(dev_a, PM_DEVICE_STATE_SUSPENDED);
	/* dev_a shall be suspended because there is no one depending on it */
	zassert_equal(ret, 0, "Device A should be suspended");

	ret = pm_device_state_set(dev_b, PM_DEVICE_STATE_SUSPENDED);
	/* dev_b shall be suspended because there is no one depending on it */
	zassert_equal(ret, 0, "Device B should be suspended");

	ret = pm_device_state_set(dev_i2c, PM_DEVICE_STATE_SUSPENDED);
	/* dev_i2c shall be suspended now because all children were
	 * suspended
	 */
	zassert_equal(ret, 0, "Device I2C should be suspended");
}

static void test_child_resume(void)
{
	int ret;
	enum pm_device_state state;

	(void)pm_device_state_get(dev_i2c, &state);
	zassert_equal(state, PM_DEVICE_STATE_SUSPENDED, "dev_i2c should be suspended");

	ret = pm_device_state_set(dev_a, PM_DEVICE_STATE_ACTIVE);
	/* dev_a shall be suspended because there is no one depending on it */
	zassert_equal(ret, 0, "Device A should be active");

	(void)pm_device_state_get(dev_i2c, &state);
	zassert_equal(state, PM_DEVICE_STATE_ACTIVE, "dev_i2c should be active");
}

static void test_ignore_children(void)
{
	bool check;
	int ret;
	enum pm_device_state state;

	check = pm_device_ignore_children_enable((struct device *)dev_i2c, true);
	zassert_true(check, "Failed to set ignore children flag");

	check = pm_device_ignore_children_is_enabled(dev_i2c);
	zassert_true(check, "Ignore children flag was not set");

	ret = pm_device_state_set(dev_i2c, PM_DEVICE_STATE_SUSPENDED);
	/* dev_i2c shall be suspended because ignore-children flag is set */
	zassert_equal(ret, 0, "Device I2C should not be suspended");

	(void)pm_device_state_get(dev_i2c, &state);
	zassert_equal(state, PM_DEVICE_STATE_SUSPENDED, "dev_i2c should be suspended");

	(void)pm_device_state_get(dev_a, &state);
	zassert_equal(state, PM_DEVICE_STATE_ACTIVE, "dev_a should still be active");
}

void test_main(void)
{
	ztest_test_suite(device_dependencies_test,
			 ztest_1cpu_unit_test(test_parent_suspend),
			 ztest_1cpu_unit_test(test_child_resume),
			 ztest_1cpu_unit_test(test_ignore_children)
		);
	ztest_run_test_suite(device_dependencies_test);
}
