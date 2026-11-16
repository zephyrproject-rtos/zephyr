/*
 * Copyright (c) 2026 Meta Platforms
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/devicetree.h>
#include <zephyr/drivers/emul.h>
#include <zephyr/drivers/i2c_emul.h>
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
	const struct sensor_value odr = {.val1 = 0x0A};
	uint8_t before = get_reg(target, BMP5_REG_ODR_CONFIG);
	uint8_t reg;

	zassert_ok(sensor_attr_set(dev, SENSOR_CHAN_ALL, SENSOR_ATTR_SAMPLING_FREQUENCY, &odr));

	reg = get_reg(target, BMP5_REG_ODR_CONFIG);
	/* ODR is bits [6:2]; verify setting and check adjacent bits are preserved */
	zassert_equal(BMP5_GET_BITSLICE(reg, BMP5_ODR), 0x0A);
	zassert_equal(reg & ~BMP5_ODR_MSK, before & ~BMP5_ODR_MSK);
}

ZTEST_F(bmp581, test_i2c_attr_odr_reg)
{
	check_attr_odr(fixture->i2c_dev, fixture->i2c_target);
}

ZTEST_F(bmp581, test_spi_attr_odr_reg)
{
	check_attr_odr(fixture->spi_dev, fixture->spi_target);
}

static bool iir_standby_failure_injected;
static uint32_t iir_dsp_accesses;

static int fail_iir_standby_write(const struct emul *target, struct i2c_msg *msgs,
				int num_msgs, int addr)
{
	ARG_UNUSED(target);
	ARG_UNUSED(addr);

	if (num_msgs < 1 || msgs[0].len < 1U) {
		return -ENOSYS;
	}

	uint8_t reg = msgs[0].buf[0];
	bool is_write = msgs[0].len > 1U ||
			(num_msgs > 1 && (msgs[1].flags & I2C_MSG_READ) == 0U);

	if (reg == BMP5_REG_ODR_CONFIG && is_write && !iir_standby_failure_injected) {
		iir_standby_failure_injected = true;
		return -EIO;
	}
	if (reg == BMP5_REG_DSP_CONFIG || reg == BMP5_REG_DSP_IIR) {
		iir_dsp_accesses++;
	}

	return -ENOSYS;
}

ZTEST_F(bmp581, test_i2c_iir_standby_error)
{
	struct i2c_emul_api mock_api = {.transfer = fail_iir_standby_write};
	const struct sensor_value mode = {.val1 = BMP5_POWERMODE_NORMAL};
	const struct sensor_value standby = {.val1 = BMP5_POWERMODE_STANDBY};
	const struct sensor_value iir = {
		.val1 = BMP581_DT_IIR_FILTER_COEFF_3,
		.val2 = BMP581_DT_IIR_FILTER_COEFF_7,
	};
	uint8_t odr_before;
	uint8_t dsp_before;
	uint8_t iir_before;
	int standby_ret;
	int ret;

	zassert_ok(sensor_attr_set(fixture->i2c_dev, SENSOR_CHAN_ALL, BMP5_ATTR_POWER_MODE,
				   &mode));
	odr_before = get_reg(fixture->i2c_target, BMP5_REG_ODR_CONFIG);
	dsp_before = get_reg(fixture->i2c_target, BMP5_REG_DSP_CONFIG);
	iir_before = get_reg(fixture->i2c_target, BMP5_REG_DSP_IIR);
	iir_standby_failure_injected = false;
	fixture->i2c_target->bus.i2c->mock_api = &mock_api;
	standby_ret = sensor_attr_set(fixture->i2c_dev, SENSOR_CHAN_ALL, BMP5_ATTR_POWER_MODE,
				      &standby);

	/* Compare against the error produced by the same transition through RTIO. */
	iir_standby_failure_injected = false;
	iir_dsp_accesses = 0U;

	ret = sensor_attr_set(fixture->i2c_dev, SENSOR_CHAN_ALL, BMP5_ATTR_IIR_CONFIG, &iir);

	fixture->i2c_target->bus.i2c->mock_api = NULL;
	zassert_true(standby_ret < 0, "Standby failure injection did not return an error");
	zassert_true(iir_standby_failure_injected);
	zassert_equal(ret, standby_ret, "Standby error was not propagated: %d", ret);
	zassert_equal(iir_dsp_accesses, 0U, "DSP registers accessed after standby failure");
	zassert_equal(get_reg(fixture->i2c_target, BMP5_REG_ODR_CONFIG), odr_before);
	zassert_equal(get_reg(fixture->i2c_target, BMP5_REG_DSP_CONFIG), dsp_before);
	zassert_equal(get_reg(fixture->i2c_target, BMP5_REG_DSP_IIR), iir_before);
}

ZTEST_F(bmp581, test_emul_get_reg_invalid)
{
	uint8_t val;

	zassert_equal(bmp581_emul_get_reg(NULL, BMP5_REG_ODR_CONFIG, &val), -EINVAL);
	zassert_equal(bmp581_emul_get_reg(fixture->i2c_target, BMP5_REG_ODR_CONFIG, NULL), -EINVAL);
	zassert_equal(bmp581_emul_get_reg(fixture->i2c_target, BMP581_EMUL_NUM_REGS, &val),
		      -EINVAL);
}
