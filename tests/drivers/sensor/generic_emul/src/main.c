/*
 * Copyright (c) 2026 Embeint Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/devicetree.h>
#include <zephyr/drivers/emul.h>
#include <zephyr/drivers/emul_sensor.h>
#include <zephyr/drivers/sensor/generic_emul.h>
#include <zephyr/kernel.h>
#include <zephyr/pm/device_runtime.h>
#include <zephyr/ztest.h>

ZTEST(drivers_sensor_generic_emul, test_init_fail)
{
	const struct device *dev = DEVICE_DT_GET(DT_NODELABEL(fail_sensor));

	zassert_false(device_is_ready(dev));
}

ZTEST(drivers_sensor_generic_emul, test_init_pass)
{
	const struct device *dev = DEVICE_DT_GET(DT_NODELABEL(test_sensor));
	const struct device *dev_pm = DEVICE_DT_GET(DT_NODELABEL(test_sensor_pm));
	const struct emul *emul = EMUL_DT_GET(DT_NODELABEL(test_sensor));

	zassert_true(device_is_ready(dev));
	zassert_true(device_is_ready(dev_pm));
	zassert_true(emul_sensor_backend_is_supported(emul));
}

ZTEST(drivers_sensor_generic_emul, test_invalid_set_get)
{
	const struct device *dev = DEVICE_DT_GET(DT_NODELABEL(test_sensor));
	const struct emul *emul = EMUL_DT_GET(DT_NODELABEL(test_sensor));
	struct sensor_chan_spec chan = {.chan_type = SENSOR_CHAN_ALL, .chan_idx = 0};
	struct sensor_value val = {0};
	q31_t sample = 0;

	zassert_equal(-ENOTSUP, emul_sensor_backend_set_channel(emul, chan, &sample, 0));
	chan.chan_type = SENSOR_CHAN_ALL + 1;
	zassert_equal(-ENOTSUP, emul_sensor_backend_set_channel(emul, chan, &sample, 0));
	chan.chan_type = SENSOR_CHAN_AMBIENT_TEMP;
	chan.chan_idx = 1;
	zassert_equal(-ENOTSUP, emul_sensor_backend_set_channel(emul, chan, &sample, 0));

	zassert_equal(-ENOTSUP, sensor_channel_get(dev, SENSOR_CHAN_ALL, &val));
	zassert_equal(-ENOTSUP, sensor_channel_get(dev, SENSOR_CHAN_ALL + 1, &val));
	zassert_equal(-ENOTSUP, sensor_sample_fetch_chan(dev, SENSOR_CHAN_ALL + 1));
}

ZTEST(drivers_sensor_generic_emul, test_fetch_rc)
{
	const struct device *dev = DEVICE_DT_GET(DT_NODELABEL(test_sensor));
	const struct emul *emul = EMUL_DT_GET(DT_NODELABEL(test_sensor));

	zassert_equal(0, sensor_sample_fetch(dev));
	generic_emul_func_rc(emul, 0, 0, -EIO);
	zassert_equal(-EIO, sensor_sample_fetch(dev));
	generic_emul_reset(emul, false);
	zassert_equal(-EIO, sensor_sample_fetch(dev));
	generic_emul_reset(emul, true);
	zassert_equal(0, sensor_sample_fetch(dev));
}

ZTEST(drivers_sensor_generic_emul, test_pm)
{
	const struct device *dev = DEVICE_DT_GET(DT_NODELABEL(test_sensor_pm));
	const struct emul *emul = EMUL_DT_GET(DT_NODELABEL(test_sensor_pm));

	if (!IS_ENABLED(CONFIG_PM_DEVICE_RUNTIME)) {
		ztest_test_skip();
		return;
	}

	generic_emul_func_rc(emul, -EIO, 0, 0);
	zassert_equal(-EIO, pm_device_runtime_get(dev));
	generic_emul_func_rc(emul, 0, -EIO, 0);
	zassert_equal(0, pm_device_runtime_get(dev));
	zassert_equal(-EIO, pm_device_runtime_put(dev));
	generic_emul_func_rc(emul, 0, 0, 0);
	zassert_equal(0, pm_device_runtime_put(dev));
}

ZTEST(drivers_sensor_generic_emul, test_value_echo)
{
	const struct device *dev = DEVICE_DT_GET(DT_NODELABEL(test_sensor));
	const struct emul *emul = EMUL_DT_GET(DT_NODELABEL(test_sensor));
	struct sensor_chan_spec chan = {.chan_idx = 0};
	struct sensor_value val_read;
	q31_t sample;
	q31_t lower;
	q31_t upper;
	q31_t epsilon;
	int8_t shift;

	zassert_equal(0, sensor_sample_fetch(dev));

	for (int i = 0; i < SENSOR_CHAN_ALL; i++) {
		zassert_equal(-ENOTSUP, sensor_channel_get(dev, i, &val_read));

		chan.chan_type = i;
		shift = 8;
		sample = ((int64_t)(i + 1) * BIT(31)) >> shift;
		zassert_equal(0, emul_sensor_backend_get_sample_range(emul, chan, &lower, &upper,
								      &epsilon, &shift));
		zassert_equal(0, emul_sensor_backend_set_channel(emul, chan, &sample, 8));

		zassert_equal(0, sensor_channel_get(dev, i, &val_read));
		zassert_equal(i + 1, val_read.val1);
		zassert_equal(0, val_read.val2);
	}

	generic_emul_reset(emul, true);
	for (int i = 0; i < SENSOR_CHAN_ALL; i++) {
		zassert_equal(-ENOTSUP, sensor_channel_get(dev, i, &val_read));
	}
}

ZTEST(drivers_sensor_generic_emul, test_fractional_and_negative_values)
{
	const struct device *dev = DEVICE_DT_GET(DT_NODELABEL(test_sensor));
	const struct emul *emul = EMUL_DT_GET(DT_NODELABEL(test_sensor));
	struct sensor_chan_spec chan = {.chan_type = SENSOR_CHAN_AMBIENT_TEMP, .chan_idx = 0};
	struct sensor_value val;
	q31_t sample;

	sample = ((int64_t)25500 * BIT(31) / 1000) >> 8;
	zassert_equal(0, emul_sensor_backend_set_channel(emul, chan, &sample, 8));
	zassert_equal(0, sensor_channel_get(dev, SENSOR_CHAN_AMBIENT_TEMP, &val));
	zassert_equal(25, val.val1);
	zassert_equal(500000, val.val2);

	sample = ((int64_t)-12500 * BIT(31) / 1000) >> 8;
	zassert_equal(0, emul_sensor_backend_set_channel(emul, chan, &sample, 8));
	zassert_equal(0, sensor_channel_get(dev, SENSOR_CHAN_AMBIENT_TEMP, &val));
	zassert_equal(-12, val.val1);
	zassert_equal(-500000, val.val2);
}

static void before_fn(void *unused)
{
	const struct emul *emul = EMUL_DT_GET(DT_NODELABEL(test_sensor));
	const struct emul *emul_pm = EMUL_DT_GET(DT_NODELABEL(test_sensor_pm));

	generic_emul_reset(emul, true);
	generic_emul_reset(emul_pm, true);
}

ZTEST_SUITE(drivers_sensor_generic_emul, NULL, NULL, before_fn, NULL, NULL);
