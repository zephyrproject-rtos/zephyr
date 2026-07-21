/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

#include <zephyr/cpu_freq/pstate.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/kernel.h>
#include <zephyr/ztest.h>

#include "cpu_freq_thermal_cap.h"

#define TEST_SENSOR_NODE DT_NODELABEL(thermal_cap_sensor)

static int32_t test_temperature_mc;
static bool test_sensor_fail;

static const struct pstate *const test_pstates[] = {
	DT_FOREACH_CHILD_STATUS_OKAY_SEP(DT_PATH(performance_states), PSTATE_DT_GET, (,))
};

static void test_temperature_set(int32_t temperature_mc)
{
	test_temperature_mc = temperature_mc;
}

static void test_sensor_fail_set(bool fail)
{
	test_sensor_fail = fail;
}

static int test_sensor_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(chan);

	return test_sensor_fail ? -EIO : 0;
}

static int test_sensor_channel_get(const struct device *dev, enum sensor_channel chan,
				   struct sensor_value *val)
{
	ARG_UNUSED(dev);

	if (chan != SENSOR_CHAN_DIE_TEMP) {
		return -ENOTSUP;
	}

	return sensor_value_from_milli(val, test_temperature_mc);
}

static DEVICE_API(sensor, test_sensor_api) = {
	.sample_fetch = test_sensor_sample_fetch,
	.channel_get = test_sensor_channel_get,
};

DEVICE_DT_DEFINE(TEST_SENSOR_NODE, NULL, NULL, NULL, NULL, POST_KERNEL,
		 CONFIG_SENSOR_INIT_PRIORITY, &test_sensor_api);

static void *cpu_freq_thermal_cap_setup(void)
{
	zassert_equal(ARRAY_SIZE(test_pstates), 3, "Expected three test P-states");

	return NULL;
}

static void cpu_freq_thermal_cap_before(void *fixture)
{
	ARG_UNUSED(fixture);

	test_sensor_fail_set(false);
	test_temperature_set(0);
	zassert_equal(cpu_freq_thermal_cap_sample(), 0, "Failed to reset thermal cap");
}

ZTEST(cpu_freq_thermal_cap, test_no_cap_below_trips)
{
	test_temperature_set(25000);
	zassert_equal(cpu_freq_thermal_cap_sample(), 0, "Failed to sample temperature");

	zassert_equal_ptr(cpu_freq_thermal_cap_apply(test_pstates[0]), test_pstates[0],
			  "P0 should be allowed below thermal trips");
	zassert_equal_ptr(cpu_freq_thermal_cap_apply(test_pstates[2]), test_pstates[2],
			  "Thermal cap must not raise a lower-performance request");
	zassert_is_null(cpu_freq_thermal_cap_apply(NULL), "NULL P-state should remain NULL");
}

ZTEST(cpu_freq_thermal_cap, test_first_trip_caps_to_p1)
{
	test_temperature_set(85000);
	zassert_equal(cpu_freq_thermal_cap_sample(), 0, "Failed to sample temperature");

	zassert_equal_ptr(cpu_freq_thermal_cap_apply(test_pstates[0]), test_pstates[1],
			  "P0 should be capped to P1 when first trip is active");
	zassert_equal_ptr(cpu_freq_thermal_cap_apply(test_pstates[1]), test_pstates[1],
			  "P1 should remain P1 when first trip is active");
	zassert_equal_ptr(cpu_freq_thermal_cap_apply(test_pstates[2]), test_pstates[2],
			  "P2 should remain P2 when first trip is active");
}

ZTEST(cpu_freq_thermal_cap, test_first_trip_hysteresis)
{
	test_temperature_set(85000);
	zassert_equal(cpu_freq_thermal_cap_sample(), 0, "Failed to activate first trip");

	test_temperature_set(76000);
	zassert_equal(cpu_freq_thermal_cap_sample(), 0, "Failed to sample hysteresis temperature");
	zassert_equal_ptr(cpu_freq_thermal_cap_apply(test_pstates[0]), test_pstates[1],
			  "First trip should remain active above release temperature");

	test_temperature_set(74000);
	zassert_equal(cpu_freq_thermal_cap_sample(), 0, "Failed to release first trip");
	zassert_equal_ptr(cpu_freq_thermal_cap_apply(test_pstates[0]), test_pstates[0],
			  "First trip should release below hysteresis threshold");
}

ZTEST(cpu_freq_thermal_cap, test_second_trip_drops_to_first_trip)
{
	test_temperature_set(96000);
	zassert_equal(cpu_freq_thermal_cap_sample(), 0, "Failed to activate second trip");
	zassert_equal_ptr(cpu_freq_thermal_cap_apply(test_pstates[0]), test_pstates[2],
			  "P0 should be capped to P2 when second trip is active");

	test_temperature_set(93000);
	zassert_equal(cpu_freq_thermal_cap_sample(), 0, "Failed to sample second trip hysteresis");
	zassert_equal_ptr(cpu_freq_thermal_cap_apply(test_pstates[0]), test_pstates[2],
			  "Second trip should remain active above its release temperature");

	test_temperature_set(91000);
	zassert_equal(cpu_freq_thermal_cap_sample(), 0, "Failed to release second trip");
	zassert_equal_ptr(cpu_freq_thermal_cap_apply(test_pstates[0]), test_pstates[1],
			  "Second trip should release back to first trip cap");
}

ZTEST(cpu_freq_thermal_cap, test_sensor_failure_uses_fail_safe_pstate)
{
	test_temperature_set(25000);
	zassert_equal(cpu_freq_thermal_cap_sample(), 0, "Failed to clear cap before failure");

	test_sensor_fail_set(true);
	for (int i = 0; i < CONFIG_CPU_FREQ_THERMAL_CAP_SENSOR_FAILURE_LIMIT; i++) {
		zassert_equal(cpu_freq_thermal_cap_sample(), -EIO,
			      "Expected fake sensor failure");
	}

	zassert_equal_ptr(cpu_freq_thermal_cap_apply(test_pstates[0]), test_pstates[2],
			  "Sensor failure should apply fail-safe P-state");

	test_sensor_fail_set(false);
	test_temperature_set(25000);
	zassert_equal(cpu_freq_thermal_cap_sample(), 0, "Failed to recover from sensor failure");
	zassert_equal_ptr(cpu_freq_thermal_cap_apply(test_pstates[0]), test_pstates[0],
			  "Sensor recovery should recompute the thermal cap");
	zassert_equal_ptr(cpu_freq_thermal_cap_apply(test_pstates[2]), test_pstates[2],
			  "Thermal cap must not raise a lower-performance request");
}

ZTEST_SUITE(cpu_freq_thermal_cap, NULL, cpu_freq_thermal_cap_setup,
	    cpu_freq_thermal_cap_before, NULL, NULL);
