/*
 * Copyright (c) 2018-2019 Peter Bigot Consulting, LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include <zephyr.h>
#include "battery.h"

/** A discharge curve calibrated from LiPo batteries.
 *
 * Specifically ones like [Adafruit 3.7v 2000
 * mAh](https://www.adafruit.com/product/2011)
 */
static const struct battery_level_point lipo[] = {

	/* "Curve" here eyeballed from captured data for a full load
	 * that started with a charge of 3.96 V and dropped about
	 * linearly to 3.58 V over 15 hours.  It then dropped rapidly
	 * to 3.10 V over one hour, at which point it stopped
	 * transmitting.
	 *
	 * Based on eyeball comparisons we'll say that 15/16 of life
	 * goes between 3.95 and 3.55 V, and 1/16 goes between 3.55 V
	 * and 3.1 V.
	 */

	{ 10000, 3950 },
	{ 625, 3550 },
	{ 0, 3100 },
};

void main(void)
{
	printk("Here we go\n");

	printk("Enable: %d\n", battery_measure_enable(true));
	while (true) {
		int batt_mV = battery_sample();
		unsigned int batt_pptt = battery_level_pptt(batt_mV, lipo);

		printk("%u: %d mV; %u pptt\n", k_uptime_get_32(),
		       batt_mV, batt_pptt);
		k_sleep(K_SECONDS(5));
	}
	printk("Disable: %d\n", battery_measure_enable(false));
}
