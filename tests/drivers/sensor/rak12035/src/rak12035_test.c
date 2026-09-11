/*
 * Copyright (c) 2026 RAKwireless Technology Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>

#include <zephyr/device.h>
#include <zephyr/drivers/emul.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/sensor/rak12035.h>
#include <zephyr/init.h>
#include <zephyr/ztest.h>

#include "rak12035_emul.h"

#define RAK12035_NEW_NODE		DT_ALIAS(rak12035_new)
#define RAK12035_OLD_DESC_NODE		DT_ALIAS(rak12035_old_desc)
#define RAK12035_OLD_ASC_NODE		DT_ALIAS(rak12035_old_asc)
#define RAK12035_OLD_INVALID_NODE	DT_ALIAS(rak12035_old_invalid)

struct rak12035_fixture {
	const struct device *new_dev;
	const struct device *old_desc_dev;
	const struct device *old_asc_dev;
	const struct device *old_invalid_dev;
	const struct emul *new_emul;
	const struct emul *old_desc_emul;
	const struct emul *old_asc_emul;
	const struct emul *old_invalid_emul;
};

static int rak12035_seed_old_firmware(void)
{
	const struct emul *old_desc = EMUL_DT_GET(DT_ALIAS(rak12035_old_desc));
	const struct emul *old_asc = EMUL_DT_GET(DT_ALIAS(rak12035_old_asc));
	const struct emul *old_invalid = EMUL_DT_GET(DT_ALIAS(rak12035_old_invalid));

	rak12035_emul_set_version(old_desc, 0x02);
	rak12035_emul_set_calibration(old_desc, 800, 300);
	rak12035_emul_set_version(old_asc, 0x02);
	rak12035_emul_set_calibration(old_asc, 100, 300);
	rak12035_emul_set_version(old_invalid, 0x02);
	rak12035_emul_set_calibration(old_invalid, 500, 500);

	return 0;
}

SYS_INIT(rak12035_seed_old_firmware, POST_KERNEL, 89);

static void *rak12035_setup(void)
{
	static struct rak12035_fixture fixture = {
		.new_dev = DEVICE_DT_GET(RAK12035_NEW_NODE),
		.old_desc_dev = DEVICE_DT_GET(RAK12035_OLD_DESC_NODE),
		.old_asc_dev = DEVICE_DT_GET(RAK12035_OLD_ASC_NODE),
		.old_invalid_dev = DEVICE_DT_GET(RAK12035_OLD_INVALID_NODE),
		.new_emul = EMUL_DT_GET(DT_ALIAS(rak12035_new)),
		.old_desc_emul = EMUL_DT_GET(DT_ALIAS(rak12035_old_desc)),
		.old_asc_emul = EMUL_DT_GET(DT_ALIAS(rak12035_old_asc)),
		.old_invalid_emul = EMUL_DT_GET(DT_ALIAS(rak12035_old_invalid)),
	};

	zassert_true(device_is_ready(fixture.new_dev));
	zassert_true(device_is_ready(fixture.old_desc_dev));
	zassert_true(device_is_ready(fixture.old_asc_dev));
	zassert_true(device_is_ready(fixture.old_invalid_dev));

	return &fixture;
}

static void rak12035_before(void *data)
{
	const struct rak12035_fixture *fixture = data;

	rak12035_emul_reset(fixture->new_emul);
	rak12035_emul_set_version(fixture->old_desc_emul, 0x02);
	rak12035_emul_set_capacitance(fixture->old_desc_emul, 550);
	rak12035_emul_set_version(fixture->old_asc_emul, 0x02);
	rak12035_emul_set_capacitance(fixture->old_asc_emul, 200);
}

ZTEST_SUITE(rak12035, NULL, rak12035_setup, rak12035_before, NULL, NULL);

ZTEST_F(rak12035, test_fetch_all_new_firmware)
{
	struct sensor_value value;

	rak12035_emul_set_temperature(fixture->new_emul, 235);
	rak12035_emul_set_capacitance(fixture->new_emul, 612);
	rak12035_emul_set_moisture(fixture->new_emul, 42);

	zassert_ok(sensor_sample_fetch(fixture->new_dev));

	zassert_ok(sensor_channel_get(fixture->new_dev, SENSOR_CHAN_AMBIENT_TEMP, &value));
	zassert_equal(value.val1, 23);
	zassert_equal(value.val2, 500000);

	zassert_ok(sensor_channel_get(fixture->new_dev, SENSOR_CHAN_HUMIDITY, &value));
	zassert_equal(value.val1, 42);
	zassert_equal(value.val2, 0);

	zassert_ok(sensor_channel_get(fixture->new_dev,
				      SENSOR_CHAN_RAK12035_CAPACITANCE_RAW, &value));
	zassert_equal(value.val1, 612);
	zassert_equal(value.val2, 0);
}

ZTEST_F(rak12035, test_negative_temperature)
{
	struct sensor_value value;

	rak12035_emul_set_temperature(fixture->new_emul, -55);

	zassert_ok(sensor_sample_fetch_chan(fixture->new_dev, SENSOR_CHAN_AMBIENT_TEMP));
	zassert_ok(sensor_channel_get(fixture->new_dev, SENSOR_CHAN_AMBIENT_TEMP, &value));
	zassert_equal(value.val1, -5);
	zassert_equal(value.val2, -500000);
}

ZTEST_F(rak12035, test_invalid_device_humidity)
{
	rak12035_emul_set_moisture(fixture->new_emul, 101);

	zassert_equal(sensor_sample_fetch_chan(fixture->new_dev, SENSOR_CHAN_HUMIDITY),
		      -ERANGE);
}

ZTEST_F(rak12035, test_i2c_error_propagation)
{
	rak12035_emul_fail_next_transfer(fixture->new_emul, -EIO);

	zassert_equal(sensor_sample_fetch_chan(fixture->new_dev, SENSOR_CHAN_HUMIDITY), -EIO);
}

ZTEST_F(rak12035, test_set_and_get_calibration)
{
	struct sensor_value dry = {.val1 = 845};
	struct sensor_value wet = {.val1 = 635};
	struct sensor_value value;

	zassert_ok(sensor_attr_set(fixture->new_dev, SENSOR_CHAN_HUMIDITY,
				   SENSOR_ATTR_RAK12035_CALIBRATION_DRY, &dry));
	zassert_ok(sensor_attr_set(fixture->new_dev, SENSOR_CHAN_HUMIDITY,
				   SENSOR_ATTR_RAK12035_CALIBRATION_WET, &wet));

	zassert_ok(sensor_attr_get(fixture->new_dev, SENSOR_CHAN_HUMIDITY,
				   SENSOR_ATTR_RAK12035_CALIBRATION_DRY, &value));
	zassert_equal(value.val1, 845);
	zassert_equal(value.val2, 0);

	zassert_ok(sensor_attr_get(fixture->new_dev, SENSOR_CHAN_HUMIDITY,
				   SENSOR_ATTR_RAK12035_CALIBRATION_WET, &value));
	zassert_equal(value.val1, 635);
	zassert_equal(value.val2, 0);
}

ZTEST_F(rak12035, test_invalid_calibration_attribute)
{
	struct sensor_value invalid_fraction = {.val1 = 500, .val2 = 1};
	struct sensor_value invalid_range = {.val1 = -1};
	struct sensor_value value = {.val1 = 500};

	zassert_equal(sensor_attr_set(fixture->new_dev, SENSOR_CHAN_HUMIDITY,
				      SENSOR_ATTR_RAK12035_CALIBRATION_DRY,
				      &invalid_fraction),
		      -EINVAL);
	zassert_equal(sensor_attr_set(fixture->new_dev, SENSOR_CHAN_HUMIDITY,
				      SENSOR_ATTR_RAK12035_CALIBRATION_DRY,
				      &invalid_range),
		      -EINVAL);
	zassert_equal(sensor_attr_set(fixture->new_dev, SENSOR_CHAN_PRESS,
				      SENSOR_ATTR_RAK12035_CALIBRATION_DRY, &value),
		      -ENOTSUP);
}

ZTEST_F(rak12035, test_old_firmware_descending_calibration)
{
	struct sensor_value value;

	zassert_ok(sensor_sample_fetch_chan(fixture->old_desc_dev, SENSOR_CHAN_HUMIDITY));
	zassert_ok(sensor_channel_get(fixture->old_desc_dev, SENSOR_CHAN_HUMIDITY, &value));
	zassert_equal(value.val1, 50);

	rak12035_emul_set_capacitance(fixture->old_desc_emul, 900);
	zassert_ok(sensor_sample_fetch_chan(fixture->old_desc_dev, SENSOR_CHAN_HUMIDITY));
	zassert_ok(sensor_channel_get(fixture->old_desc_dev, SENSOR_CHAN_HUMIDITY, &value));
	zassert_equal(value.val1, 0);

	rak12035_emul_set_capacitance(fixture->old_desc_emul, 200);
	zassert_ok(sensor_sample_fetch_chan(fixture->old_desc_dev, SENSOR_CHAN_HUMIDITY));
	zassert_ok(sensor_channel_get(fixture->old_desc_dev, SENSOR_CHAN_HUMIDITY, &value));
	zassert_equal(value.val1, 100);
}

ZTEST_F(rak12035, test_old_firmware_ascending_calibration)
{
	struct sensor_value value;

	zassert_ok(sensor_sample_fetch_chan(fixture->old_asc_dev, SENSOR_CHAN_HUMIDITY));
	zassert_ok(sensor_channel_get(fixture->old_asc_dev, SENSOR_CHAN_HUMIDITY, &value));
	zassert_equal(value.val1, 50);

	rak12035_emul_set_capacitance(fixture->old_asc_emul, 50);
	zassert_ok(sensor_sample_fetch_chan(fixture->old_asc_dev, SENSOR_CHAN_HUMIDITY));
	zassert_ok(sensor_channel_get(fixture->old_asc_dev, SENSOR_CHAN_HUMIDITY, &value));
	zassert_equal(value.val1, 0);

	rak12035_emul_set_capacitance(fixture->old_asc_emul, 350);
	zassert_ok(sensor_sample_fetch_chan(fixture->old_asc_dev, SENSOR_CHAN_HUMIDITY));
	zassert_ok(sensor_channel_get(fixture->old_asc_dev, SENSOR_CHAN_HUMIDITY, &value));
	zassert_equal(value.val1, 100);
}

ZTEST_F(rak12035, test_unsupported_channel)
{
	struct sensor_value value;

	zassert_equal(sensor_sample_fetch_chan(fixture->new_dev, SENSOR_CHAN_PRESS), -ENOTSUP);
	zassert_equal(sensor_channel_get(fixture->new_dev, SENSOR_CHAN_PRESS, &value), -ENOTSUP);
}
