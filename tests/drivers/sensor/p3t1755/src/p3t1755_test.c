/*
 * Copyright 2026 NXP
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/emul.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/pm/device.h>
#include <zephyr/ztest.h>

#include "p3t1755.h"
#include "p3t1755_emul.h"

struct p3t1755_fixture {
	const struct device *dev;
	const struct emul *emul;
};

struct p3t1755_oneshot_fixture {
	const struct device *dev;
	const struct emul *emul;
};

static struct p3t1755_fixture continuous_fixture = {
	.dev = DEVICE_DT_GET(DT_ALIAS(p3t1755)),
	.emul = EMUL_DT_GET(DT_ALIAS(p3t1755)),
};

static struct p3t1755_oneshot_fixture oneshot_fixture = {
	.dev = DEVICE_DT_GET(DT_ALIAS(p3t1755_oneshot)),
	.emul = EMUL_DT_GET(DT_ALIAS(p3t1755_oneshot)),
};

static void *p3t1755_setup(void)
{
	zassert_true(device_is_ready(continuous_fixture.dev));
	zassert_not_null(continuous_fixture.emul);

	return &continuous_fixture;
}

static void *p3t1755_oneshot_setup(void)
{
	zassert_true(device_is_ready(oneshot_fixture.dev));
	zassert_not_null(oneshot_fixture.emul);

	return &oneshot_fixture;
}

static void p3t1755_reset(const struct device *dev, const struct emul *target)
{
	struct p3t1755_data *data = dev->data;

	p3t1755_emul_reset(target);
	data->sample = 0;
	data->config_reg = 0;
}

static void p3t1755_before(void *fixture_ptr)
{
	const struct p3t1755_fixture *fixture = fixture_ptr;

	p3t1755_reset(fixture->dev, fixture->emul);
}

static void p3t1755_oneshot_before(void *fixture_ptr)
{
	const struct p3t1755_oneshot_fixture *fixture = fixture_ptr;

	p3t1755_reset(fixture->dev, fixture->emul);
}

ZTEST_SUITE(p3t1755, NULL, p3t1755_setup, p3t1755_before, NULL, NULL);
ZTEST_SUITE(p3t1755_oneshot, NULL, p3t1755_oneshot_setup, p3t1755_oneshot_before, NULL,
	    NULL);

static void verify_temperature(const struct p3t1755_fixture *fixture, int16_t raw,
			       int32_t val1, int32_t val2)
{
	struct sensor_value value;

	p3t1755_emul_set_temperature(fixture->emul, raw);
	zassert_ok(sensor_sample_fetch(fixture->dev));
	zassert_ok(sensor_channel_get(fixture->dev, SENSOR_CHAN_AMBIENT_TEMP, &value));
	zassert_equal(val1, value.val1);
	zassert_equal(val2, value.val2);
}

ZTEST_F(p3t1755, test_temperature_conversion)
{
	verify_temperature(fixture, 0, 0, 0);
	verify_temperature(fixture, 408, 25, 500000);
	verify_temperature(fixture, -1, 0, -62500);
	verify_temperature(fixture, -640, -40, 0);
	verify_temperature(fixture, 2047, 127, 937500);
	verify_temperature(fixture, -2048, -128, 0);
}

ZTEST_F(p3t1755, test_supported_fetch_channels)
{
	p3t1755_emul_set_temperature(fixture->emul, 16);

	zassert_ok(sensor_sample_fetch(fixture->dev));
	zassert_ok(sensor_sample_fetch_chan(fixture->dev, SENSOR_CHAN_AMBIENT_TEMP));
}

ZTEST_F(p3t1755, test_unsupported_channels)
{
	struct sensor_value value;

	zassert_equal(-ENOTSUP, sensor_sample_fetch_chan(fixture->dev, SENSOR_CHAN_PRESS));
	zassert_equal(-ENOTSUP,
		      sensor_channel_get(fixture->dev, SENSOR_CHAN_PRESS, &value));
}

ZTEST_F(p3t1755, test_read_error_is_propagated)
{
	p3t1755_emul_set_read_failure(fixture->emul, true);
	zassert_equal(-EIO, sensor_sample_fetch(fixture->dev));
}

ZTEST_F(p3t1755, test_channel_get_uses_last_sample)
{
	struct sensor_value value;

	p3t1755_emul_set_temperature(fixture->emul, 16);
	zassert_ok(sensor_sample_fetch(fixture->dev));
	p3t1755_emul_set_temperature(fixture->emul, 32);

	zassert_ok(sensor_channel_get(fixture->dev, SENSOR_CHAN_AMBIENT_TEMP, &value));
	zassert_equal(1, value.val1);
	zassert_equal(0, value.val2);
}

ZTEST_F(p3t1755_oneshot, test_fetch_starts_one_shot_conversion)
{
	p3t1755_emul_set_temperature(fixture->emul, 400);

	zassert_ok(sensor_sample_fetch(fixture->dev));
	zassert_equal(P3T1755_CONFIG_REG_OS, p3t1755_emul_get_config(fixture->emul));
	zassert_equal(1, p3t1755_emul_get_config_write_count(fixture->emul));
}

ZTEST_F(p3t1755_oneshot, test_one_shot_write_error_is_propagated)
{
	p3t1755_emul_set_write_failure(fixture->emul, true);
	zassert_equal(-EIO, sensor_sample_fetch(fixture->dev));
}

ZTEST_F(p3t1755_oneshot, test_resume_configures_shutdown_mode)
{
	const uint8_t initial_config = BIT(2);

	p3t1755_emul_set_config(fixture->emul, initial_config);
	zassert_ok(pm_device_action_run(fixture->dev, PM_DEVICE_ACTION_SUSPEND));
	zassert_ok(pm_device_action_run(fixture->dev, PM_DEVICE_ACTION_RESUME));
	zassert_equal(initial_config | P3T1755_CONFIG_REG_SD,
		      p3t1755_emul_get_config(fixture->emul));
	zassert_equal(1, p3t1755_emul_get_config_write_count(fixture->emul));
}

ZTEST_F(p3t1755_oneshot, test_resume_read_error_is_propagated)
{
	zassert_ok(pm_device_action_run(fixture->dev, PM_DEVICE_ACTION_SUSPEND));
	p3t1755_emul_set_read_failure(fixture->emul, true);
	zassert_equal(-EIO, pm_device_action_run(fixture->dev, PM_DEVICE_ACTION_RESUME));

	p3t1755_emul_set_read_failure(fixture->emul, false);
	zassert_ok(pm_device_action_run(fixture->dev, PM_DEVICE_ACTION_RESUME));
}

ZTEST_F(p3t1755_oneshot, test_resume_write_error_is_propagated)
{
	zassert_ok(pm_device_action_run(fixture->dev, PM_DEVICE_ACTION_SUSPEND));
	p3t1755_emul_set_write_failure(fixture->emul, true);
	zassert_equal(-EIO, pm_device_action_run(fixture->dev, PM_DEVICE_ACTION_RESUME));

	p3t1755_emul_set_write_failure(fixture->emul, false);
	zassert_ok(pm_device_action_run(fixture->dev, PM_DEVICE_ACTION_RESUME));
}

ZTEST_F(p3t1755_oneshot, test_unsupported_pm_action)
{
	zassert_equal(-ENOTSUP,
		      pm_device_action_run(fixture->dev, PM_DEVICE_ACTION_TURN_OFF));
}
