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

ZTEST(icm20948, test_unsupported_channel_is_rejected)
{
	struct sensor_value val;

	zassert_ok(sensor_sample_fetch(dev));
	zassert_equal(sensor_channel_get(dev, SENSOR_CHAN_PRESS, &val), -ENOTSUP,
		      "an unsupported channel should be rejected");
}

ZTEST_SUITE(icm20948, NULL, NULL, NULL, NULL, NULL);
