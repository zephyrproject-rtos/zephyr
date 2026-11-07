/*
 * Copyright (c) 2026 Ali Talha Yurtseven
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/drivers/emul.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/ztest.h>

#include "icm20948_emul.h"
#include "icm20948_reg.h"

/* Registers of the magnetometer, which the driver keeps to itself */
#define AK09916_REG_ST1   0x10
#define AK09916_REG_HXL   0x11
#define AK09916_REG_ST2   0x18
#define AK09916_REG_CNTL2 0x31

#define BIT_AK09916_HOFL 0x08

/* 0.15 uT per LSB, which the driver reports as micro Gauss */
#define AK09916_SCALE_TO_UG 1500

#define NODE DT_NODELABEL(icm20948)

static const struct device *dev = DEVICE_DT_GET(NODE);
static const struct emul *target = EMUL_DT_GET(NODE);

/* Places one big endian sample in the burst the device reads out in one go */
static void set_sample(uint16_t reg, int16_t val)
{
	uint8_t raw[2];

	sys_put_be16((uint16_t)val, raw);
	icm20948_emul_set_reg(target, reg, raw, sizeof(raw));
}

ZTEST(icm20948, test_device_is_ready)
{
	zassert_true(device_is_ready(dev), "the driver did not initialise");
}

ZTEST(icm20948, test_init_leaves_the_device_awake)
{
	uint8_t pwr;

	icm20948_emul_get_reg(target, REG_PWR_MGMT_1, &pwr, sizeof(pwr));

	zassert_equal(pwr & BIT_SLEEP, 0, "the device was left asleep");
	zassert_equal(FIELD_GET(MASK_CLKSEL, pwr), CLKSEL_AUTO,
		      "the device was not left on its own clock");
}

ZTEST(icm20948, test_accel_is_read_big_endian)
{
	struct sensor_value val[3];

	/* One g on Z at the reset full scale range of two g */
	set_sample(REG_ACCEL_XOUT_H, 0);
	set_sample(REG_ACCEL_XOUT_H + 2, 0);
	set_sample(REG_ACCEL_XOUT_H + 4, 16384);

	zassert_ok(sensor_sample_fetch(dev));
	zassert_ok(sensor_channel_get(dev, SENSOR_CHAN_ACCEL_XYZ, val));

	zassert_equal(val[0].val1, 0, "x should read zero");
	zassert_equal(val[1].val1, 0, "y should read zero");
	zassert_within(sensor_value_to_double(&val[2]), 9.80665, 0.01, "z should read one g");
}

ZTEST(icm20948, test_negative_accel_keeps_its_sign)
{
	struct sensor_value val;

	set_sample(REG_ACCEL_XOUT_H + 4, -16384);

	zassert_ok(sensor_sample_fetch(dev));
	zassert_ok(sensor_channel_get(dev, SENSOR_CHAN_ACCEL_Z, &val));

	zassert_within(sensor_value_to_double(&val), -9.80665, 0.01, "z should read minus one g");
}

/* The magnetometer is little endian, unlike the rest of the device */
static void set_magn(int16_t x, int16_t y, int16_t z)
{
	uint8_t raw[6];

	sys_put_le16((uint16_t)x, &raw[0]);
	sys_put_le16((uint16_t)y, &raw[2]);
	sys_put_le16((uint16_t)z, &raw[4]);

	icm20948_emul_set_magn_reg(target, AK09916_REG_HXL, raw, sizeof(raw));
}

static void set_magn_st2(uint8_t st2)
{
	icm20948_emul_set_magn_reg(target, AK09916_REG_ST2, &st2, sizeof(st2));
}

ZTEST(icm20948, test_magnetometer_is_started_in_a_continuous_mode)
{
	uint8_t cntl2 = 0;

	/*
	 * The driver reaches the magnetometer over slave 4, so reading its
	 * register file back proves the auxiliary bus carried the write.
	 */
	icm20948_emul_get_magn_reg(target, AK09916_REG_CNTL2, &cntl2, sizeof(cntl2));

	zassert_not_equal(cntl2, 0, "the magnetometer was left powered down");
}

ZTEST(icm20948, test_magn_is_read_little_endian)
{
	struct sensor_value val[3];

	/* Y and Z are inverted on the way out, so give them a sign to lose */
	set_magn(1000, 2000, -3000);
	set_magn_st2(0);

	zassert_ok(sensor_sample_fetch(dev));
	zassert_ok(sensor_channel_get(dev, SENSOR_CHAN_MAGN_XYZ, val));

	zassert_within(sensor_value_to_double(&val[0]), 1000.0 * AK09916_SCALE_TO_UG / 1000000.0,
		       0.001, "x should follow the sample");
	zassert_within(sensor_value_to_double(&val[1]), -2000.0 * AK09916_SCALE_TO_UG / 1000000.0,
		       0.001, "y should be inverted");
	zassert_within(sensor_value_to_double(&val[2]), 3000.0 * AK09916_SCALE_TO_UG / 1000000.0,
		       0.001, "z should be inverted");
}

ZTEST(icm20948, test_magn_overflow_is_reported)
{
	struct sensor_value val;

	set_magn(1000, 2000, 3000);
	set_magn_st2(BIT_AK09916_HOFL);

	zassert_ok(sensor_sample_fetch(dev));
	zassert_equal(sensor_channel_get(dev, SENSOR_CHAN_MAGN_X, &val), -EOVERFLOW,
		      "a sample out of range should be refused");
}

ZTEST(icm20948, test_unsupported_channel_is_rejected)
{
	struct sensor_value val;

	zassert_ok(sensor_sample_fetch(dev));
	zassert_equal(sensor_channel_get(dev, SENSOR_CHAN_PRESS, &val), -ENOTSUP,
		      "an unsupported channel should be rejected");
}

ZTEST_SUITE(icm20948, NULL, NULL, NULL, NULL, NULL);
