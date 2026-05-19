/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdbool.h>
#include <stdint.h>

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>

#define DIE_TEMP_NODE DT_ALIAS(die_temp0)
#define THERMAL_CAP_NODE DT_NODELABEL(cpu_freq_thermal_cap)
#define TRIP0_NODE DT_CHILD(THERMAL_CAP_NODE, trip_0)
#define TRIP1_NODE DT_CHILD(THERMAL_CAP_NODE, trip_1)

#define SAMPLE_PERIOD_MS 1000
#define SAMPLE_BUSY_MS 800

#define TRIP0_TEMP_MC DT_PROP(TRIP0_NODE, temperature_millicelsius)
#define TRIP0_RELEASE_MC \
	(TRIP0_TEMP_MC - DT_PROP_OR(TRIP0_NODE, hysteresis_millicelsius, 0))
#define TRIP0_CAP_NAME DT_NODE_FULL_NAME(DT_PHANDLE(TRIP0_NODE, cap_pstate))

#define TRIP1_TEMP_MC DT_PROP(TRIP1_NODE, temperature_millicelsius)
#define TRIP1_RELEASE_MC \
	(TRIP1_TEMP_MC - DT_PROP_OR(TRIP1_NODE, hysteresis_millicelsius, 0))
#define TRIP1_CAP_NAME DT_NODE_FULL_NAME(DT_PHANDLE(TRIP1_NODE, cap_pstate))

static const struct device *const die_temp = DEVICE_DT_GET(DIE_TEMP_NODE);
static bool trip0_active;
static bool trip1_active;

static void print_temperature(int32_t temperature_mc)
{
	if (temperature_mc < 0) {
		printk("-");
		temperature_mc = -temperature_mc;
	}

	printk("%d.%03d C", temperature_mc / 1000, temperature_mc % 1000);
}

static void update_trip(bool *active, int32_t temperature_mc, int32_t threshold_mc,
				int32_t release_mc)
{
	if (!*active && (temperature_mc >= threshold_mc)) {
		*active = true;
	}

	if (*active && (temperature_mc <= release_mc)) {
		*active = false;
	}
}

static const char *thermal_cap_name(int32_t temperature_mc)
{
	update_trip(&trip0_active, temperature_mc, TRIP0_TEMP_MC, TRIP0_RELEASE_MC);
	update_trip(&trip1_active, temperature_mc, TRIP1_TEMP_MC, TRIP1_RELEASE_MC);

	if (trip1_active) {
		return TRIP1_CAP_NAME;
	}

	if (trip0_active) {
		return TRIP0_CAP_NAME;
	}

	return "none";
}

static void print_trip(const char *name, int32_t threshold_mc, int32_t release_mc,
		       const char *cap_name)
{
	printk("%s: ", name);
	print_temperature(threshold_mc);
	printk(" -> cap %s, release at ", cap_name);
	print_temperature(release_mc);
	printk("\n");
}

static int log_die_temperature(void)
{
	struct sensor_value temperature;
	int32_t temperature_mc;
	int ret;

	ret = sensor_sample_fetch(die_temp);
	if (ret != 0) {
		printk("failed to fetch die temperature: %d\n", ret);
		return ret;
	}

	ret = sensor_channel_get(die_temp, SENSOR_CHAN_DIE_TEMP, &temperature);
	if (ret != 0) {
		printk("failed to read die temperature: %d\n", ret);
		return ret;
	}

	temperature_mc = (int32_t)sensor_value_to_milli(&temperature);

	printk("die temperature: ");
	print_temperature(temperature_mc);
	printk(", thermal cap: %s\n", thermal_cap_name(temperature_mc));

	return 0;
}

int main(void)
{
	if (!device_is_ready(die_temp)) {
		printk("die temperature sensor is not ready\n");
		return 0;
	}

	printk("CPUFreq thermal cap sample\n");
	print_trip("trip 0", TRIP0_TEMP_MC, TRIP0_RELEASE_MC, TRIP0_CAP_NAME);
	print_trip("trip 1", TRIP1_TEMP_MC, TRIP1_RELEASE_MC, TRIP1_CAP_NAME);
	printk("\n");

	while (1) {
		(void)log_die_temperature();
		k_busy_wait(SAMPLE_BUSY_MS * USEC_PER_MSEC);
		k_msleep(SAMPLE_PERIOD_MS - SAMPLE_BUSY_MS);
	}
}
