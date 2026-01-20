/*
 * Copyright (c) 2026 Om Srivastava <srivastavaom97714@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/i2c_emul.h>
#include <zephyr/sys/byteorder.h>
#include <errno.h>
#include <zephyr/drivers/sensor/bh1750_emul.h>

/* ================= Emulator ================= */

extern const struct i2c_emul_api bh1750_emul_api;

void bh1750_emul_set_value(const struct device *dev, void *raw_data);

/* ================= Test Fixture ================= */

#define BH1750_NODE DT_INST(0, rohm_bh1750)
#define BH1750_ADDR DT_REG_ADDR(BH1750_NODE)

struct i2c_emul emul_i2c = {
	.api = &bh1750_emul_api,
	.addr = BH1750_ADDR,
};

struct bh1750_fixture {
	struct bh1750_emul_data emul_data;
	struct emul emul;
};

static void *bh1750_setup(void)
{
	static struct bh1750_fixture fixture = {0};

	const struct device *i2c = DEVICE_DT_GET(DT_BUS(BH1750_NODE));

	zassert_true(device_is_ready(i2c), "I2C bus not ready");

	fixture.emul_data.powered = true;
	fixture.emul_data.raw = 0;

	fixture.emul.dev = DEVICE_DT_GET(BH1750_NODE);
	fixture.emul.bus.i2c = &emul_i2c;
	fixture.emul.bus.i2c->target = &fixture.emul;
	fixture.emul.data = &fixture.emul_data;

	/* 🔑 Runtime registration */
	zassert_true(device_is_ready(fixture.emul.dev),
		     "BH1750 device not ready");

	return &fixture;
}

static void bh1750_before(void *f)
{
	struct bh1750_fixture *fixture = f;

	fixture->emul_data.powered = true;
	fixture->emul_data.raw = 0;
}

ZTEST_SUITE(bh1750, NULL,
	    bh1750_setup, bh1750_before,
	    NULL, NULL);

/* ================= Tests ================= */

ZTEST_F(bh1750, test_sample_fetch_ok)
{
	zassert_ok(sensor_sample_fetch(fixture->emul.dev));
}

ZTEST_F(bh1750, test_100_lux)
{
	struct sensor_value val;
	/* raw = lux * 1.2 → 100 lux */
	fixture->emul_data.raw = 120;
	bh1750_emul_set_value(fixture->emul.dev, &fixture->emul_data);

	zassert_ok(sensor_sample_fetch(fixture->emul.dev));
	zassert_ok(sensor_channel_get(fixture->emul.dev,
				     SENSOR_CHAN_LIGHT, &val));

	zassert_equal(val.val1, 100);
	zassert_equal(val.val2, 0);
}

ZTEST_F(bh1750, test_fractional_lux)
{
	struct sensor_value val;

	/* 12.5 lux → raw = 15 */
	fixture->emul_data.raw = 15;
	bh1750_emul_set_value(fixture->emul.dev, &fixture->emul_data);

	zassert_ok(sensor_sample_fetch(fixture->emul.dev));
	zassert_ok(sensor_channel_get(fixture->emul.dev,
				     SENSOR_CHAN_LIGHT, &val));

	zassert_equal(val.val1, 12);
	zassert_equal(val.val2, 500000);
}
