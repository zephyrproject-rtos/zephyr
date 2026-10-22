/*
 * Copyright (c) 2026 Meta Platforms
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/devicetree.h>
#include <zephyr/drivers/emul.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/ztest.h>

#include "bmp581_emul.h"

struct bmp581_fixture {
	const struct device *i2c_dev;
	const struct emul *i2c_target;
	const struct device *spi_dev;
	const struct emul *spi_target;
};

static void *bmp581_setup(void)
{
	static struct bmp581_fixture fixture = {
		.i2c_dev = DEVICE_DT_GET(DT_NODELABEL(bmp581)),
		.i2c_target = EMUL_DT_GET(DT_NODELABEL(bmp581)),
		.spi_dev = DEVICE_DT_GET(DT_NODELABEL(bmp581_spi)),
		.spi_target = EMUL_DT_GET(DT_NODELABEL(bmp581_spi)),
	};

	zassert_not_null(fixture.i2c_dev);
	zassert_not_null(fixture.i2c_target);
	zassert_not_null(fixture.spi_dev);
	zassert_not_null(fixture.spi_target);
	return &fixture;
}

static void bmp581_before(void *f)
{
	struct bmp581_fixture *fixture = f;

	bmp581_emul_set_temp_raw(fixture->i2c_target, 0);
	bmp581_emul_set_press_raw(fixture->i2c_target, 0);
	bmp581_emul_set_temp_raw(fixture->spi_target, 0);
	bmp581_emul_set_press_raw(fixture->spi_target, 0);
}

ZTEST_SUITE(bmp581, NULL, bmp581_setup, bmp581_before, NULL, NULL);

static void check_fetch_temp(const struct device *dev, const struct emul *target)
{
	/* raw 1638400 -> 1638400 / 65536 = 25.0 degC */
	const int32_t raw_temp = 1638400;
	struct sensor_value val;

	bmp581_emul_set_temp_raw(target, raw_temp);

	zassert_ok(sensor_sample_fetch(dev));
	zassert_ok(sensor_channel_get(dev, SENSOR_CHAN_AMBIENT_TEMP, &val));

	zassert_within(sensor_value_to_double(&val), (double)raw_temp / 65536.0, 0.01,
		       "temperature mismatch: %d.%06d", val.val1, val.val2);
}

static void check_fetch_press(const struct device *dev, const struct emul *target)
{
	/* raw 6400000 -> (6400000 >> 6) / 1000 = 100.0 kPa */
	const uint32_t raw_press = 6400000;
	struct sensor_value val;

	bmp581_emul_set_press_raw(target, raw_press);

	zassert_ok(sensor_sample_fetch(dev));
	zassert_ok(sensor_channel_get(dev, SENSOR_CHAN_PRESS, &val));

	zassert_within(sensor_value_to_double(&val), (double)(raw_press >> 6) / 1000.0, 0.01,
		       "pressure mismatch: %d.%06d", val.val1, val.val2);
}

/* The drivers must have initialised against the emulated chips (soft-reset, chip-id, NVM). */
ZTEST_F(bmp581, test_i2c_device_ready)
{
	zassert_true(device_is_ready(fixture->i2c_dev), "BMP581 (I2C) device not ready");
}

ZTEST_F(bmp581, test_i2c_fetch_temp)
{
	check_fetch_temp(fixture->i2c_dev, fixture->i2c_target);
}

ZTEST_F(bmp581, test_i2c_fetch_press)
{
	check_fetch_press(fixture->i2c_dev, fixture->i2c_target);
}

ZTEST_F(bmp581, test_i2c_attr_set_sampling_frequency)
{
	struct sensor_value odr = {.val1 = 50, .val2 = 0};

	zassert_ok(sensor_attr_set(fixture->i2c_dev, SENSOR_CHAN_ALL,
				   SENSOR_ATTR_SAMPLING_FREQUENCY, &odr));
}

ZTEST_F(bmp581, test_spi_device_ready)
{
	zassert_true(device_is_ready(fixture->spi_dev), "BMP581 (SPI) device not ready");
}

ZTEST_F(bmp581, test_spi_fetch_temp)
{
	check_fetch_temp(fixture->spi_dev, fixture->spi_target);
}

ZTEST_F(bmp581, test_spi_fetch_press)
{
	check_fetch_press(fixture->spi_dev, fixture->spi_target);
}
