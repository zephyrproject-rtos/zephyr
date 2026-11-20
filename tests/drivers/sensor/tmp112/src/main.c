/*
 * Copyright (c) 2026, Iomico
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>

#include <zephyr/device.h>
#include <zephyr/drivers/emul.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/ztest.h>

#include "tmp112.h"
#include "tmp112_emul.h"

struct tmp112_fixture {
	const struct device *dev;
	struct tmp112_emul_data *data;
};

static void *tmp112_setup(void)
{
	static struct tmp112_fixture fixture = {
		.dev = DEVICE_DT_GET(DT_NODELABEL(tmp112)),
	};
	const struct emul *emul = EMUL_DT_GET(DT_NODELABEL(tmp112));

	zassert_true(device_is_ready(fixture.dev), "TMP112 initialization failed");
	fixture.data = emul->data;

	return &fixture;
}

static void tmp112_before(void *fixture_ptr)
{
	struct tmp112_fixture *fixture = fixture_ptr;
	const struct sensor_value frequency = {0};
	struct sensor_value temperature;

	/* Restore both driver and emulator state through the public API before each test. */
	fixture->data->fail_config_write = false;
	zassert_ok(sensor_attr_set(fixture->dev, SENSOR_CHAN_AMBIENT_TEMP,
				   SENSOR_ATTR_SAMPLING_FREQUENCY, &frequency));
	zassert_true((fixture->data->regs[TMP112_REG_CONFIG] & TMP112_CONFIG_SD) != 0U);
	fixture->data->one_shot_count = 0;
	zassert_ok(sensor_sample_fetch(fixture->dev));
	zassert_equal(fixture->data->one_shot_count, 1, "Initial one-shot conversion missing");
	zassert_ok(sensor_channel_get(fixture->dev, SENSOR_CHAN_AMBIENT_TEMP, &temperature));
	zassert_equal(temperature.val1, 25);
	zassert_equal(temperature.val2, 0);
	fixture->data->config_write_count = 0;
	fixture->data->one_shot_count = 0;
}

ZTEST_F(tmp112, test_invalid_frequency_preserves_one_shot)
{
	const struct sensor_value frequency = {.val1 = 2};
	uint16_t config = fixture->data->regs[TMP112_REG_CONFIG];

	zassert_equal(sensor_attr_set(fixture->dev, SENSOR_CHAN_AMBIENT_TEMP,
				      SENSOR_ATTR_SAMPLING_FREQUENCY, &frequency),
		      -ENOTSUP);
	zassert_equal(fixture->data->config_write_count, 0,
		      "Unsupported frequency attempted a configuration write");
	zassert_equal(fixture->data->regs[TMP112_REG_CONFIG], config);
	zassert_ok(sensor_sample_fetch(fixture->dev));
	zassert_equal(fixture->data->one_shot_count, 1,
		      "Fetch did not trigger one-shot after rejecting the frequency");
}

ZTEST_F(tmp112, test_config_write_error_preserves_one_shot)
{
	const struct sensor_value frequency = {.val1 = 4};
	uint16_t config = fixture->data->regs[TMP112_REG_CONFIG];

	fixture->data->fail_config_write = true;
	zassert_equal(sensor_attr_set(fixture->dev, SENSOR_CHAN_AMBIENT_TEMP,
				      SENSOR_ATTR_SAMPLING_FREQUENCY, &frequency),
		      -EIO);
	zassert_equal(fixture->data->config_write_count, 1,
		      "Valid frequency did not attempt a configuration write");
	zassert_equal(fixture->data->regs[TMP112_REG_CONFIG], config);
	fixture->data->fail_config_write = false;
	zassert_ok(sensor_sample_fetch(fixture->dev));
	zassert_equal(fixture->data->one_shot_count, 1,
		      "Fetch did not trigger one-shot after the configuration write failed");
}

ZTEST_F(tmp112, test_config_write_error_preserves_periodic)
{
	const struct sensor_value periodic = {.val1 = 4};
	const struct sensor_value one_shot = {0};
	uint16_t config;

	zassert_ok(sensor_attr_set(fixture->dev, SENSOR_CHAN_AMBIENT_TEMP,
				   SENSOR_ATTR_SAMPLING_FREQUENCY, &periodic));
	config = fixture->data->regs[TMP112_REG_CONFIG];
	zassert_equal(config & TMP112_CONFIG_SD, 0);

	fixture->data->fail_config_write = true;
	zassert_equal(sensor_attr_set(fixture->dev, SENSOR_CHAN_AMBIENT_TEMP,
				      SENSOR_ATTR_SAMPLING_FREQUENCY, &one_shot),
		      -EIO);
	zassert_equal(fixture->data->config_write_count, 2,
		      "One-shot request did not attempt a configuration write");
	zassert_equal(fixture->data->regs[TMP112_REG_CONFIG], config);
	fixture->data->fail_config_write = false;
	zassert_ok(sensor_sample_fetch(fixture->dev));
	/* The sensor is still converting continuously, so fetch must not write OS. */
	zassert_equal(fixture->data->config_write_count, 2,
		      "Periodic fetch wrote the configuration after the write failed");
	zassert_equal(fixture->data->one_shot_count, 0);
}

ZTEST_F(tmp112, test_successful_sampling_mode_transitions)
{
	const struct sensor_value periodic = {.val1 = 4};
	const struct sensor_value one_shot = {0};

	zassert_ok(sensor_attr_set(fixture->dev, SENSOR_CHAN_AMBIENT_TEMP,
				   SENSOR_ATTR_SAMPLING_FREQUENCY, &periodic));
	zassert_equal(fixture->data->regs[TMP112_REG_CONFIG] & TMP112_CONFIG_SD, 0);
	zassert_equal(fixture->data->regs[TMP112_REG_CONFIG] & TMP112_CONV_RATE_MASK,
		      TMP112_CONV_RATE(TMP112_CONV_RATE_4));
	zassert_equal(fixture->data->config_write_count, 1);
	zassert_ok(sensor_sample_fetch(fixture->dev));
	/* A periodic fetch must not write OS, even if shutdown is already cleared. */
	zassert_equal(fixture->data->config_write_count, 1,
		      "Periodic fetch unexpectedly wrote the configuration");
	zassert_equal(fixture->data->one_shot_count, 0);

	zassert_ok(sensor_attr_set(fixture->dev, SENSOR_CHAN_AMBIENT_TEMP,
				   SENSOR_ATTR_SAMPLING_FREQUENCY, &one_shot));
	zassert_true((fixture->data->regs[TMP112_REG_CONFIG] & TMP112_CONFIG_SD) != 0U);
	zassert_equal(fixture->data->config_write_count, 2);
	zassert_equal(fixture->data->one_shot_count, 0);
	zassert_ok(sensor_sample_fetch(fixture->dev));
	zassert_equal(fixture->data->config_write_count, 3);
	zassert_equal(fixture->data->one_shot_count, 1,
		      "Fetch did not trigger a conversion after returning to one-shot");
}

ZTEST_SUITE(tmp112, NULL, tmp112_setup, tmp112_before, NULL, NULL);
