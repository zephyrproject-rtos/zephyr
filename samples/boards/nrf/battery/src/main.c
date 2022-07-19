/*
 * Copyright (c) 2018-2019 Peter Bigot Consulting, LLC
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include <zephyr/zephyr.h>
#include "battery.h"

/** A discharge curve specific to the power source. */
static const struct battery_level_point levels[] = {
	/* "Curve" here eyeballed from captured data for the [Adafruit
	 * 3.7v 2000 mAh](https://www.adafruit.com/product/2011) LIPO
	 * under full load that started with a charge of 3.96 V and
	 * dropped about linearly to 3.58 V over 15 hours.  It then
	 * dropped rapidly to 3.10 V over one hour, at which point it
	 * stopped transmitting.
	 *
	 * Based on eyeball comparisons we'll say that 15/16 of life
	 * goes between 3.95 and 3.55 V, and 1/16 goes between 3.55 V
	 * and 3.1 V.
	 */

	{ 10000, 3950 },
	{ 625, 3550 },
	{ 0, 3100 },
};

static const char *now_str(void)
{
	static char buf[16]; /* ...HH:MM:SS.MMM */
	uint32_t now = k_uptime_get_32();
	unsigned int ms = now % MSEC_PER_SEC;
	unsigned int s;
	unsigned int min;
	unsigned int h;

	now /= MSEC_PER_SEC;
	s = now % 60U;
	now /= 60U;
	min = now % 60U;
	now /= 60U;
	h = now;

	snprintf(buf, sizeof(buf), "%u:%02u:%02u.%03u",
		 h, min, s, ms);
	return buf;
}

void main(void)
{
	int rc = battery_measure_enable(true);

	if (rc != 0) {
		printk("Failed initialize battery measurement: %d\n", rc);
		return;
	}

	while (true) {
		int batt_mV = battery_sample();

		if (batt_mV < 0) {
			printk("Failed to read battery voltage: %d\n",
			       batt_mV);
			break;
		}

		unsigned int batt_pptt = battery_level_pptt(batt_mV, levels);

		printk("[%s]: %d mV; %u pptt\n", now_str(),
		       batt_mV, batt_pptt);

		/* Burn battery so you can see that this works over time */
		k_busy_wait(5 * USEC_PER_SEC);
	}
	printk("Disable: %d\n", battery_measure_enable(false));
}
