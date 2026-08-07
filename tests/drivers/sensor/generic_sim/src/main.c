/*
 * Copyright (c) 2026 Embeint Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/kernel.h>
#include <zephyr/devicetree.h>
#include <zephyr/pm/device_runtime.h>

#include <zephyr/drivers/sensor/generic_sim.h>

ZTEST(drivers_sensor_generic_sim, test_init_fail)
{
	const struct device *dev = DEVICE_DT_GET(DT_NODELABEL(fail_sensor));

	zassert_false(device_is_ready(dev));
}

ZTEST(drivers_sensor_generic_sim, test_init_pass)
{
	const struct device *dev = DEVICE_DT_GET(DT_NODELABEL(test_sensor));
	const struct device *dev_pm = DEVICE_DT_GET(DT_NODELABEL(test_sensor_pm));

	zassert_true(device_is_ready(dev));
	zassert_true(device_is_ready(dev_pm));
}

ZTEST(drivers_sensor_generic_sim, test_invalid_set_get)
{
	const struct device *dev = DEVICE_DT_GET(DT_NODELABEL(test_sensor));
	struct sensor_value val = {0};

	zassert_equal(-EINVAL, generic_sim_channel_set(dev, SENSOR_CHAN_ALL, val));
	zassert_equal(-EINVAL, generic_sim_channel_set(dev, SENSOR_CHAN_ALL + 1, val));
	zassert_equal(-ENOTSUP, sensor_channel_get(dev, SENSOR_CHAN_ALL, &val));
	zassert_equal(-ENOTSUP, sensor_channel_get(dev, SENSOR_CHAN_ALL + 1, &val));

	zassert_equal(-ENOTSUP, sensor_sample_fetch_chan(dev, SENSOR_CHAN_ALL + 1));
}

ZTEST(drivers_sensor_generic_sim, test_fetch_rc)
{
	const struct device *dev = DEVICE_DT_GET(DT_NODELABEL(test_sensor));

	zassert_equal(0, sensor_sample_fetch(dev));
	generic_sim_func_rc(dev, 0, 0, -EIO);
	zassert_equal(-EIO, sensor_sample_fetch(dev));
	generic_sim_reset(dev, false);
	zassert_equal(-EIO, sensor_sample_fetch(dev));
	generic_sim_reset(dev, true);
	zassert_equal(0, sensor_sample_fetch(dev));
}

ZTEST(drivers_sensor_generic_sim, test_pm)
{
	const struct device *dev = DEVICE_DT_GET(DT_NODELABEL(test_sensor_pm));

	if (!IS_ENABLED(CONFIG_PM_DEVICE_RUNTIME)) {
		ztest_test_skip();
		return;
	}

	generic_sim_func_rc(dev, -EIO, 0, 0);
	zassert_equal(-EIO, pm_device_runtime_get(dev));
	generic_sim_func_rc(dev, 0, -EIO, 0);
	zassert_equal(0, pm_device_runtime_get(dev));
	zassert_equal(-EIO, pm_device_runtime_put(dev));
	generic_sim_func_rc(dev, 0, 0, 0);
	zassert_equal(0, pm_device_runtime_put(dev));
}

ZTEST(drivers_sensor_generic_sim, test_value_echo)
{
	const struct device *dev = DEVICE_DT_GET(DT_NODELABEL(test_sensor));
	struct sensor_value val_write, val_read;

	zassert_equal(0, sensor_sample_fetch(dev));

	for (int i = 0; i < SENSOR_CHAN_ALL; i++) {
		/* Not supported before configuring */
		zassert_equal(-ENOTSUP, sensor_channel_get(dev, i, &val_read));
		/* Can configure */
		val_write.val1 = i + 1;
		val_write.val2 = i - 10;
		zassert_equal(0, generic_sim_channel_set(dev, i, val_write));
		/* Returns the expected value */
		zassert_equal(0, sensor_channel_get(dev, i, &val_read));
		zassert_equal(val_write.val1, val_read.val1);
		zassert_equal(val_write.val2, val_read.val2);
	}

	/* Reset clears all channels */
	generic_sim_reset(dev, true);
	for (int i = 0; i < SENSOR_CHAN_ALL; i++) {
		zassert_equal(-ENOTSUP, sensor_channel_get(dev, i, &val_read));
	}
}

static void before_fn(void *unused)
{
	const struct device *dev = DEVICE_DT_GET(DT_NODELABEL(test_sensor));

	generic_sim_reset(dev, true);
}

ZTEST_SUITE(drivers_sensor_generic_sim, NULL, NULL, before_fn, NULL, NULL);
