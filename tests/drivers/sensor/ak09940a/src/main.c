/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/emul.h>
#include <zephyr/drivers/gpio/gpio_emul.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/sensor_data_types.h>
#include <zephyr/rtio/rtio.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/ztest.h>

#include "ak09940a_emul.h"
#include "ak09940a_reg.h"

#define I2C_NODE DT_NODELABEL(ak09940a_i2c)
#define SPI_NODE DT_NODELABEL(ak09940a_spi)

#define RAW_MAX 131070
#define RAW_MIN (-131072)

struct ak09940a_fixture {
	const struct device *dev;
	const struct emul *target;
	struct rtio_iodev *iodev;
	/* CNTL3 drive bits and fastest continuous mode of the configured sensor drive */
	uint8_t cntl3_mt;
	int32_t max_odr;
	uint8_t max_mode;
};

SENSOR_DT_READ_IODEV(ak09940a_i2c_iodev, I2C_NODE, {SENSOR_CHAN_MAGN_XYZ, 0},
		     {SENSOR_CHAN_DIE_TEMP, 0});
SENSOR_DT_READ_IODEV(ak09940a_spi_iodev, SPI_NODE, {SENSOR_CHAN_MAGN_XYZ, 0},
		     {SENSOR_CHAN_DIE_TEMP, 0});

/* The executor frees a submission after posting its completion, so allow one in flight */
RTIO_DEFINE(ak09940a_rtio, 2, 2);

static const struct ak09940a_fixture fixtures[] = {
	{
		/* low-noise-2 */
		.dev = DEVICE_DT_GET(I2C_NODE),
		.target = EMUL_DT_GET(I2C_NODE),
		.iodev = &ak09940a_i2c_iodev,
		.cntl3_mt = 0x60,
		.max_odr = 200,
		.max_mode = AK09940A_MODE_CONT_200HZ,
	},
	{
		/* ultra-low-power */
		.dev = DEVICE_DT_GET(SPI_NODE),
		.target = EMUL_DT_GET(SPI_NODE),
		.iodev = &ak09940a_spi_iodev,
		.cntl3_mt = 0x00,
		.max_odr = 2500,
		.max_mode = AK09940A_MODE_CONT_2500HZ,
	},
};

static void set_measurement(const struct emul *target, int32_t x, int32_t y, int32_t z, int8_t tmps)
{
	uint8_t regs[10];

	/* 18-bit two's complement, the high byte repeats the sign bit */
	sys_put_le24((uint32_t)x, &regs[0]);
	sys_put_le24((uint32_t)y, &regs[3]);
	sys_put_le24((uint32_t)z, &regs[6]);
	regs[9] = (uint8_t)tmps;
	ak09940a_emul_set_reg(target, AK09940A_REG_HXL, regs, sizeof(regs));
}

static uint8_t get_reg(const struct emul *target, uint8_t reg)
{
	uint8_t val;

	ak09940a_emul_get_reg(target, reg, &val, 1);

	return val;
}

static void assert_value(const struct sensor_value *val, int32_t val1, int32_t val2)
{
	zassert_equal(val->val1, val1, "val1: expected %d, got %d", val1, val->val1);
	zassert_equal(val->val2, val2, "val2: expected %d, got %d", val2, val->val2);
}

static int64_t q31_to_micro(q31_t value, int8_t shift)
{
	return (((int64_t)value << shift) * 1000000) / (INT64_C(1) << 31);
}

static void ak09940a_after(void *f)
{
	const struct sensor_value off = {0};

	ARG_UNUSED(f);

	ARRAY_FOR_EACH_PTR(fixtures, fixture) {
		zassert_ok(sensor_attr_set(fixture->dev, SENSOR_CHAN_MAGN_XYZ,
					   SENSOR_ATTR_SAMPLING_FREQUENCY, &off));
	}
}

ZTEST_SUITE(ak09940a, NULL, NULL, NULL, ak09940a_after, NULL);

ZTEST(ak09940a, test_init)
{
	const struct device *gpio = DEVICE_DT_GET(DT_GPIO_CTLR(I2C_NODE, reset_gpios));

	ARRAY_FOR_EACH_PTR(fixtures, fixture) {
		zassert_true(device_is_ready(fixture->dev));
	}

	/* Reset line released */
	zassert_equal(gpio_emul_output_get(gpio, DT_GPIO_PIN(I2C_NODE, reset_gpios)), 1);

	zassert_equal(get_reg(fixtures[0].target, AK09940A_REG_CNTL1), 0);
	zassert_equal(get_reg(fixtures[0].target, AK09940A_REG_I2CDIS), 0);

	/* Ultra low power drive, I2C interface disabled on SPI */
	zassert_equal(get_reg(fixtures[1].target, AK09940A_REG_CNTL1), AK09940A_CNTL1_MT2);
	zassert_equal(get_reg(fixtures[1].target, AK09940A_REG_I2CDIS), AK09940A_I2CDIS_DISABLE);
}

ZTEST(ak09940a, test_fetch_single)
{
	struct sensor_value magn[3];
	struct sensor_value val;

	ARRAY_FOR_EACH_PTR(fixtures, fixture) {
		set_measurement(fixture->target, RAW_MAX, RAW_MIN, -1, 0);

		zassert_ok(sensor_sample_fetch(fixture->dev));

		/* Single measurement started with the drive bits, then back to power-down */
		zassert_equal(get_reg(fixture->target, AK09940A_REG_CNTL3), fixture->cntl3_mt);
		zassert_equal(get_reg(fixture->target, AK09940A_REG_ST1) & AK099XX_ST1_DRDY, 0);

		/* 10 nT (0.1 mG) per LSB */
		zassert_ok(sensor_channel_get(fixture->dev, SENSOR_CHAN_MAGN_XYZ, magn));
		assert_value(&magn[0], 13, 107000);
		assert_value(&magn[1], -13, -107200);
		assert_value(&magn[2], 0, -100);

		zassert_ok(sensor_channel_get(fixture->dev, SENSOR_CHAN_MAGN_Y, &val));
		assert_value(&val, -13, -107200);

		zassert_ok(sensor_channel_get(fixture->dev, SENSOR_CHAN_DIE_TEMP, &val));
		assert_value(&val, 30, 0);
	}
}

ZTEST(ak09940a, test_fetch_temperature)
{
	/* Temperature = 30 - TMPS / 1.7 */
	static const struct {
		int8_t tmps;
		int32_t val1;
		int32_t val2;
	} cases[] = {
		{1, 29, 411765},
		{-1, 30, 588235},
		{INT8_MAX, -44, -705882},
		{INT8_MIN, 105, 294117},
	};
	struct sensor_value val;

	ARRAY_FOR_EACH_PTR(fixtures, fixture) {
		ARRAY_FOR_EACH_PTR(cases, c) {
			set_measurement(fixture->target, 0, 0, 0, c->tmps);
			zassert_ok(sensor_sample_fetch(fixture->dev));
			zassert_ok(sensor_channel_get(fixture->dev, SENSOR_CHAN_DIE_TEMP, &val));
			assert_value(&val, c->val1, c->val2);
		}
	}
}

ZTEST(ak09940a, test_fetch_continuous)
{
	const struct sensor_value odr = {.val1 = 100};
	const uint8_t st1 = 0;
	struct sensor_value val;

	ARRAY_FOR_EACH_PTR(fixtures, fixture) {
		zassert_ok(sensor_attr_set(fixture->dev, SENSOR_CHAN_MAGN_XYZ,
					   SENSOR_ATTR_SAMPLING_FREQUENCY, &odr));
		zassert_equal(get_reg(fixture->target, AK09940A_REG_CNTL3),
			      fixture->cntl3_mt | AK099XX_MODE_CONT_100HZ);

		/* The data registers hold the last measurement even without DRDY */
		set_measurement(fixture->target, 1000, 0, 0, 0);
		ak09940a_emul_set_reg(fixture->target, AK09940A_REG_ST1, &st1, 1);

		zassert_ok(sensor_sample_fetch(fixture->dev));
		zassert_ok(sensor_channel_get(fixture->dev, SENSOR_CHAN_MAGN_X, &val));
		assert_value(&val, 0, 100000);

		/* No new single measurement was started */
		zassert_equal(get_reg(fixture->target, AK09940A_REG_CNTL3),
			      fixture->cntl3_mt | AK099XX_MODE_CONT_100HZ);
	}
}

ZTEST(ak09940a, test_sampling_frequency)
{
	/* Requests round down to a supported rate, capped by the sensor drive */
	static const struct {
		int32_t request;
		int32_t hz;
		uint8_t mode;
	} cases[] = {
		{0, 0, AK099XX_MODE_POWER_DOWN},
		{1, 10, AK099XX_MODE_CONT_10HZ},
		{20, 20, AK099XX_MODE_CONT_20HZ},
		{99, 50, AK099XX_MODE_CONT_50HZ},
		{150, 100, AK099XX_MODE_CONT_100HZ},
		{200, 200, AK09940A_MODE_CONT_200HZ},
		{999, 400, AK09940A_MODE_CONT_400HZ},
		{1000, 1000, AK09940A_MODE_CONT_1000HZ},
		{5000, 2500, AK09940A_MODE_CONT_2500HZ},
	};
	struct sensor_value val;

	ARRAY_FOR_EACH_PTR(fixtures, fixture) {
		ARRAY_FOR_EACH_PTR(cases, c) {
			const struct sensor_value odr = {.val1 = c->request};
			bool capped = c->hz > fixture->max_odr;
			int32_t hz = capped ? fixture->max_odr : c->hz;
			uint8_t mode = capped ? fixture->max_mode : c->mode;

			zassert_ok(sensor_attr_set(fixture->dev, SENSOR_CHAN_MAGN_XYZ,
						   SENSOR_ATTR_SAMPLING_FREQUENCY, &odr));
			zassert_ok(sensor_attr_get(fixture->dev, SENSOR_CHAN_MAGN_XYZ,
						   SENSOR_ATTR_SAMPLING_FREQUENCY, &val));
			assert_value(&val, hz, 0);
			zassert_equal(get_reg(fixture->target, AK09940A_REG_CNTL3),
				      fixture->cntl3_mt | mode, "request %d Hz", c->request);
		}

		zassert_equal(sensor_attr_set(fixture->dev, SENSOR_CHAN_MAGN_XYZ,
					      SENSOR_ATTR_FULL_SCALE, &val),
			      -ENOTSUP);
	}
}

ZTEST(ak09940a, test_read_decode)
{
	struct sensor_chan_spec magn_spec = {SENSOR_CHAN_MAGN_XYZ, 0};
	struct sensor_chan_spec temp_spec = {SENSOR_CHAN_DIE_TEMP, 0};
	const struct sensor_decoder_api *decoder;
	struct sensor_three_axis_data magn;
	struct sensor_q31_data temp;
	uint8_t buf[64];
	uint32_t fit;

	ARRAY_FOR_EACH_PTR(fixtures, fixture) {
		set_measurement(fixture->target, RAW_MAX, RAW_MIN, -1, INT8_MAX);

		zassert_ok(sensor_read(fixture->iodev, &ak09940a_rtio, buf, sizeof(buf)));
		zassert_ok(sensor_get_decoder(fixture->dev, &decoder));

		fit = 0;
		zassert_equal(decoder->decode(buf, magn_spec, &fit, 1, &magn), 1);
		zassert_within(q31_to_micro(magn.readings[0].x, magn.shift), 13107000, 1);
		zassert_within(q31_to_micro(magn.readings[0].y, magn.shift), -13107200, 1);
		zassert_within(q31_to_micro(magn.readings[0].z, magn.shift), -100, 1);

		fit = 0;
		zassert_equal(decoder->decode(buf, temp_spec, &fit, 1, &temp), 1);
		zassert_within(q31_to_micro(temp.readings[0].temperature, temp.shift), -44705882,
			       1);
	}
}
