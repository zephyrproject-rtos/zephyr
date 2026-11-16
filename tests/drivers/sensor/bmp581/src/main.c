/*
 * Copyright (c) 2026 Meta Platforms
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/devicetree.h>
#include <zephyr/drivers/emul.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/ztest.h>
#include "bmp581_emul.h"
#include "bmp581.h"

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

static uint8_t get_reg(const struct emul *target, uint8_t reg)
{
	uint8_t val = 0;

	zassert_ok(bmp581_emul_get_reg(target, reg, &val));

	return val;
}

static void check_attr_odr(const struct device *dev, const struct emul *target)
{
	const struct bmp581_data *data = dev->data;
	const struct {
		struct sensor_value frequency;
		uint8_t encoding;
	} cases[] = {
		{{50, 0}, BMP581_DT_ODR_50_HZ},
		{{100, 200000}, BMP581_DT_ODR_100_2_HZ},
		{{218, 500000}, BMP581_DT_ODR_218_5_HZ},
		{{240, 0}, BMP581_DT_ODR_240_HZ},
		{{1, 0}, BMP581_DT_ODR_1_HZ},
		{{0, 500000}, BMP581_DT_ODR_0_5_HZ},
		{{0, 250000}, BMP581_DT_ODR_0_250_HZ},
		{{0, 125000}, BMP581_DT_ODR_0_125_HZ},
		{{0, 125001}, BMP581_DT_ODR_0_250_HZ},
		{{0, 1}, BMP581_DT_ODR_0_125_HZ},
	};

	for (size_t i = 0U; i < ARRAY_SIZE(cases); i++) {
		uint8_t before = get_reg(target, BMP5_REG_ODR_CONFIG);
		uint8_t reg;

		zassert_ok(sensor_attr_set(dev, SENSOR_CHAN_ALL, SENSOR_ATTR_SAMPLING_FREQUENCY,
					  &cases[i].frequency));

		reg = get_reg(target, BMP5_REG_ODR_CONFIG);
		zassert_equal(BMP5_GET_BITSLICE(reg, BMP5_ODR), cases[i].encoding,
			      "Unexpected ODR for %d.%06d Hz", cases[i].frequency.val1,
			      cases[i].frequency.val2);
		zassert_equal(reg & ~BMP5_ODR_MSK, before & ~BMP5_ODR_MSK);
		zassert_equal(data->osr_odr_press_config.odr, cases[i].encoding);
	}
}

ZTEST_F(bmp581, test_i2c_attr_odr_reg)
{
	check_attr_odr(fixture->i2c_dev, fixture->i2c_target);
}

ZTEST_F(bmp581, test_spi_attr_odr_reg)
{
	check_attr_odr(fixture->spi_dev, fixture->spi_target);
}

static void check_invalid_frequency(const struct device *dev, const struct emul *target)
{
	const struct bmp581_data *data = dev->data;
	const struct sensor_value frequencies[] = {
		{0, 0},
		{-1, 0},
		{0, -1},
		{240, 1},
		{241, 0},
		{INT32_MAX, 0},
	};
	const struct sensor_value valid = {.val1 = 50};
	uint8_t before;

	zassert_ok(sensor_attr_set(dev, SENSOR_CHAN_ALL, SENSOR_ATTR_SAMPLING_FREQUENCY, &valid));
	before = get_reg(target, BMP5_REG_ODR_CONFIG);

	for (size_t i = 0U; i < ARRAY_SIZE(frequencies); i++) {
		zassert_equal(sensor_attr_set(dev, SENSOR_CHAN_ALL, SENSOR_ATTR_SAMPLING_FREQUENCY,
					     &frequencies[i]), -EINVAL);
		zassert_equal(get_reg(target, BMP5_REG_ODR_CONFIG), before);
		zassert_equal(data->osr_odr_press_config.odr, BMP581_DT_ODR_50_HZ);
	}
}

ZTEST_F(bmp581, test_i2c_invalid_sampling_frequency)
{
	check_invalid_frequency(fixture->i2c_dev, fixture->i2c_target);
}

ZTEST_F(bmp581, test_spi_invalid_sampling_frequency)
{
	check_invalid_frequency(fixture->spi_dev, fixture->spi_target);
}

ZTEST_F(bmp581, test_emul_get_reg_invalid)
{
	uint8_t val;

	zassert_equal(bmp581_emul_get_reg(NULL, BMP5_REG_ODR_CONFIG, &val), -EINVAL);
	zassert_equal(bmp581_emul_get_reg(fixture->i2c_target, BMP5_REG_ODR_CONFIG, NULL), -EINVAL);
	zassert_equal(bmp581_emul_get_reg(fixture->i2c_target, BMP581_EMUL_NUM_REGS, &val),
		      -EINVAL);
}
