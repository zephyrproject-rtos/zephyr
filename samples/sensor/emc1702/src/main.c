/*
 * Copyright (c) 2026  Microchip Technology Inc. and its subsidiaries
 * SPDX-License-Identifier: Apache-2.0
 *
 * EMC1702 sensor driver sample app
 *
 * Periodically reads die temperature, ambient temperature, bus voltage,
 * current, and power from the EMC1702 via the Zephyr sensor API, and
 * prints decoded values to the console.
 */

#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>

#define SAMPLE_PERIOD_MS 1000

/* 10^n lookup for scaling the micro (val2) part down to the requested decimals. */
static const int32_t pow10[] = {1, 10, 100, 1000, 10000, 100000, 1000000};

static void print_chan(const char *label, const struct sensor_value *v, const char *unit,
		       int decimals)
{
	/* sensor_value: val1 = integer part, val2 = micro part (signed). */
	int32_t frac_part = v->val2;
	int32_t int_part = v->val1;
	int sign = 1;

	if (int_part < 0 || frac_part < 0) {
		sign = -1;
		if (int_part < 0) {
			int_part = -int_part;
		}
		if (frac_part < 0) {
			frac_part = -frac_part;
		}
	}

	int32_t frac_scaled = frac_part / pow10[6 - decimals];

	printk("%s= %s%d.%0*d %s  ", label, sign < 0 ? "-" : "", (int)int_part, decimals,
	       (int)frac_scaled, unit);
}

int main(void)
{
	const struct device *const emc = DEVICE_DT_GET_ONE(microchip_emc1702);

	if (!device_is_ready(emc)) {
		printk("EMC1702 not ready\n");
		return -1;
	}
	printk("EMC1702 ready, sampling every %d ms\n", SAMPLE_PERIOD_MS);

	uint32_t t = 0;

	while (1) {
		struct sensor_value temp, temp_amb, vbus, current, power;
		int rc = sensor_sample_fetch(emc);

		if (rc) {
			printk("[%6u ms] fetch failed: %d\n", t, rc);
			goto wait;
		}
		sensor_channel_get(emc, SENSOR_CHAN_DIE_TEMP, &temp);
		sensor_channel_get(emc, SENSOR_CHAN_AMBIENT_TEMP, &temp_amb);
		sensor_channel_get(emc, SENSOR_CHAN_VOLTAGE, &vbus);
		sensor_channel_get(emc, SENSOR_CHAN_CURRENT, &current);
		sensor_channel_get(emc, SENSOR_CHAN_POWER, &power);

		printk("[%6u ms]  ", t);
		print_chan("Tdie", &temp, "C", 2);
		print_chan("Tamb", &temp_amb, "C", 2);
		print_chan("Vbus", &vbus, "V", 3);
		print_chan("I", &current, "A", 4);
		print_chan("P", &power, "W", 4);
		printk("\n");

wait:
		k_msleep(SAMPLE_PERIOD_MS);
		t += SAMPLE_PERIOD_MS;
	}
	return 0;
}
