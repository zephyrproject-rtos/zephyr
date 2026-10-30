/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Linumiz
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/init.h>
#include <zephyr/pmbus/pmbus_cmd.h>
#include <zephyr/ztest.h>

#include "fake_pmbus_dev.h"

static const struct device *const sensor = DEVICE_DT_GET(DT_NODELABEL(pmbus));

static struct fake_pmbus_dev fake;

static int boot_clear_faults;

static int64_t get_milli(enum sensor_channel chan)
{
	struct sensor_value val;

	zassert_ok(sensor_channel_get(sensor, chan, &val));

	return sensor_value_to_milli(&val);
}

ZTEST(pmbus_sensor, test_init)
{
	zassert_true(device_is_ready(sensor));
	zassert_equal(boot_clear_faults, 0);
}

ZTEST(pmbus_sensor, test_fetch_all)
{
	struct sensor_value val;

	fake.regs[0][PMBUS_READ_VOUT] = 0x0D9A;
	fake.regs[0][PMBUS_READ_IOUT] = 0xE0C8;
	fake.regs[0][PMBUS_READ_POUT] = 0x0828;
	fake.regs[0][PMBUS_READ_TEMPERATURE_1] = 0xF84B;

	zassert_ok(sensor_sample_fetch(sensor));

	zassert_ok(sensor_channel_get(sensor, SENSOR_CHAN_VOLTAGE, &val));
	zassert_equal(val.val1, 6);
	zassert_equal(val.val2, 801000);

	zassert_equal(get_milli(SENSOR_CHAN_CURRENT), 12500);
	zassert_equal(get_milli(SENSOR_CHAN_POWER), 80000);
	zassert_equal(get_milli(SENSOR_CHAN_AMBIENT_TEMP), 37500);

	zassert_equal(fake.clear_faults, 0);
	zassert_equal(fake.cml, 0);
}

ZTEST(pmbus_sensor, test_fetch_channel)
{
	fake.regs[0][PMBUS_READ_VOUT] = 0x0D9A;
	fake.regs[0][PMBUS_READ_IOUT] = 0xE0C8;
	zassert_ok(sensor_sample_fetch(sensor));

	fake.regs[0][PMBUS_READ_VOUT] = 0x0000;
	fake.regs[0][PMBUS_READ_IOUT] = 0x0000;
	zassert_ok(sensor_sample_fetch_chan(sensor, SENSOR_CHAN_CURRENT));

	zassert_equal(get_milli(SENSOR_CHAN_CURRENT), 0);
	zassert_equal(get_milli(SENSOR_CHAN_VOLTAGE), 6801);
}

ZTEST(pmbus_sensor, test_fetch_error)
{
	struct sensor_value val;

	zassert_equal(sensor_sample_fetch_chan(sensor, SENSOR_CHAN_DIE_TEMP), -ENOTSUP);
	zassert_equal(sensor_channel_get(sensor, SENSOR_CHAN_DIE_TEMP, &val), -ENOTSUP);

	atomic_set_bit(fake.nack, PMBUS_READ_IOUT);
	zassert_equal(sensor_sample_fetch(sensor), -EIO);
	zassert_equal(sensor_sample_fetch_chan(sensor, SENSOR_CHAN_CURRENT), -EIO);
	zassert_ok(sensor_sample_fetch_chan(sensor, SENSOR_CHAN_VOLTAGE));

	fake.absent = true;
	zassert_equal(sensor_sample_fetch_chan(sensor, SENSOR_CHAN_VOLTAGE), -EIO);
}

static void before(void *fixture)
{
	ARG_UNUSED(fixture);

	fake_pmbus_dev_reset(&fake);
}

static void *setup(void)
{
	boot_clear_faults = fake.clear_faults;

	return NULL;
}

/* The sensor probes the device in its init, so register the fake before */
static int register_fake(void)
{
	return fake_pmbus_dev_register(&fake, DEVICE_DT_GET(DT_NODELABEL(i2c1)),
				       DT_REG_ADDR(DT_NODELABEL(pmbus)), 1, 0x17);
}

BUILD_ASSERT(CONFIG_I2C_INIT_PRIORITY < 60 && CONFIG_SENSOR_INIT_PRIORITY > 60);
SYS_INIT(register_fake, POST_KERNEL, 60);

ZTEST_SUITE(pmbus_sensor, NULL, setup, before, NULL, NULL);
