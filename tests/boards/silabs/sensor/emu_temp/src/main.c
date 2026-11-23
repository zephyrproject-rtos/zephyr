/*
 * Copyright (c) 2026 Nahuel Mesa <nahuel.mesa@focus.uy>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/kernel.h>
#include <zephyr/ztest.h>

/* EFR32xG21 commercial/industrial operating range, with margin for die
 * self-heating above the ambient maximum. Bounds this wide only catch gross
 * implementation errors (e.g. returning raw register counts or Kelvin
 * instead of Celsius), not accuracy regressions.
 */
#define EMU_TEMP_MIN_C -40.0
#define EMU_TEMP_MAX_C 110.0

static const struct device *const emu_temp_dev = DEVICE_DT_GET(DT_NODELABEL(temp));

static void before(void *fixture)
{
	ARG_UNUSED(fixture);

	zassert_true(device_is_ready(emu_temp_dev), "Device %s is not ready.", emu_temp_dev->name);
}

ZTEST(emu_temp, test_sample_fetch_all)
{
	zassert_ok(sensor_sample_fetch(emu_temp_dev));
}

ZTEST(emu_temp, test_sample_fetch_die_temp)
{
	zassert_ok(sensor_sample_fetch_chan(emu_temp_dev, SENSOR_CHAN_DIE_TEMP));
}

ZTEST(emu_temp, test_sample_fetch_unsupported_channel)
{
	zassert_equal(sensor_sample_fetch_chan(emu_temp_dev, SENSOR_CHAN_HUMIDITY), -ENOTSUP);
}

ZTEST(emu_temp, test_channel_get_unsupported_channel)
{
	struct sensor_value val;

	zassert_ok(sensor_sample_fetch(emu_temp_dev));
	zassert_equal(sensor_channel_get(emu_temp_dev, SENSOR_CHAN_HUMIDITY, &val), -ENOTSUP);
}

ZTEST(emu_temp, test_channel_get_plausible_range)
{
	struct sensor_value val;
	double temp_c;

	zassert_ok(sensor_sample_fetch(emu_temp_dev));
	zassert_ok(sensor_channel_get(emu_temp_dev, SENSOR_CHAN_DIE_TEMP, &val));

	temp_c = sensor_value_to_double(&val);
	TC_PRINT("Die temperature: %.2f C\n", temp_c);

	zassert_true(temp_c > EMU_TEMP_MIN_C, "Temperature %.2f C below minimum", temp_c);
	zassert_true(temp_c < EMU_TEMP_MAX_C, "Temperature %.2f C above maximum", temp_c);
}

ZTEST_SUITE(emu_temp, NULL, NULL, before, NULL, NULL);
