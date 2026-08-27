/*
 * Copyright (c) 2021 Leica Geosystems AG
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/sensor.h>
#include <zephyr/kernel.h>
#include <zephyr/ztest.h>

const struct device *get_fuel_gauge_device(void)
{
	const struct device *const dev = DEVICE_DT_GET_ANY(sbs_sbs_gauge);

	zassert_true(device_is_ready(dev), "Fuel Gauge not found");

	return dev;
}

/* Helper that only checks channel access is supported. */
static void test_get_sensor_value(int16_t channel)
{
	struct sensor_value value;
	const struct device *dev = get_fuel_gauge_device();

	zassert_ok(sensor_sample_fetch_chan(dev, channel), "Sample fetch failed");
	zassert_ok(sensor_channel_get(dev, channel, &value), "Get sensor value failed");
}

/* Helper for verifying a sensor channel fetch is not supported */
static void test_get_sensor_value_not_supp(int16_t channel)
{
	const struct device *dev = get_fuel_gauge_device();

	zassert_true(sensor_sample_fetch_chan(dev, channel) == -ENOTSUP, "Invalid function");
}

ZTEST(sbs_gauge, test_get_gauge_voltage)
{
	test_get_sensor_value(SENSOR_CHAN_GAUGE_VOLTAGE);
}

ZTEST(sbs_gauge, test_get_gauge_avg_current)
{
	test_get_sensor_value(SENSOR_CHAN_GAUGE_AVG_CURRENT);
}

ZTEST(sbs_gauge, test_get_gauge_get_temperature)
{
	test_get_sensor_value(SENSOR_CHAN_GAUGE_TEMP);
}

ZTEST(sbs_gauge, test_get_state_of_charge)
{
	test_get_sensor_value(SENSOR_CHAN_GAUGE_STATE_OF_CHARGE);
}

ZTEST(sbs_gauge, test_get_full_charge_capacity)
{
	test_get_sensor_value(SENSOR_CHAN_GAUGE_FULL_CHARGE_CAPACITY);
}

ZTEST(sbs_gauge, test_get_rem_charge_capacity)
{
	test_get_sensor_value(SENSOR_CHAN_GAUGE_REMAINING_CHARGE_CAPACITY);
}

ZTEST(sbs_gauge, test_get_nom_avail_capacity)
{
	test_get_sensor_value(SENSOR_CHAN_GAUGE_NOM_AVAIL_CAPACITY);
}

ZTEST(sbs_gauge, test_get_full_avail_capacity)
{
	test_get_sensor_value(SENSOR_CHAN_GAUGE_FULL_AVAIL_CAPACITY);
}

ZTEST(sbs_gauge, test_get_average_time_to_empty)
{
	test_get_sensor_value(SENSOR_CHAN_GAUGE_TIME_TO_EMPTY);
}

ZTEST(sbs_gauge, test_get_average_time_to_full)
{
	test_get_sensor_value(SENSOR_CHAN_GAUGE_TIME_TO_FULL);
}

ZTEST(sbs_gauge, test_get_cycle_count)
{
	test_get_sensor_value(SENSOR_CHAN_GAUGE_CYCLE_COUNT);
}

ZTEST(sbs_gauge, test_not_supported_channel)
{
	uint8_t channel;

	for (channel = SENSOR_CHAN_ACCEL_X; channel <= SENSOR_CHAN_RPM; channel++) {
		test_get_sensor_value_not_supp(channel);
	}
	/* SOH is not defined in the SBS 1.1 specifications */
	test_get_sensor_value_not_supp(SENSOR_CHAN_GAUGE_STATE_OF_HEALTH);

	/* These readings are not presently supported by the sbs_gauge driver. */
	test_get_sensor_value_not_supp(SENSOR_CHAN_GAUGE_STDBY_CURRENT);
	test_get_sensor_value_not_supp(SENSOR_CHAN_GAUGE_MAX_LOAD_CURRENT);
	test_get_sensor_value_not_supp(SENSOR_CHAN_GAUGE_DESIRED_VOLTAGE);
	test_get_sensor_value_not_supp(SENSOR_CHAN_GAUGE_DESIRED_CHARGING_CURRENT);
}

ZTEST_SUITE(sbs_gauge, NULL, NULL, NULL, NULL, NULL);
