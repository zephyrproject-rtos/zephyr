/*
 * Copyright (c) 2018-2019 Peter Bigot Consulting, LLC
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <stdlib.h>

#include <zephyr.h>

#include "battery_level.h"

unsigned int battery_level_pptt(unsigned int batt_mV,
				const struct battery_level_point *curve)
{
	const struct battery_level_point *pb = curve;

	if (batt_mV >= pb->lvl_mV) {
		/* Measured voltage above highest point, cap at maximum. */
		return pb->lvl_pptt;
	}
	/* Go down to the last point at or below the measured voltage. */
	while ((pb->lvl_pptt > 0)
	       && (batt_mV < pb->lvl_mV)) {
		++pb;
	}
	if (batt_mV < pb->lvl_mV) {
		/* Below lowest point, cap at minimum */
		return pb->lvl_pptt;
	}

	/* Linear interpolation between below and above points. */
	const struct battery_level_point *pa = pb - 1;

	return pb->lvl_pptt
	       + ((pa->lvl_pptt - pb->lvl_pptt)
		  * (batt_mV - pb->lvl_mV)
		  / (pa->lvl_mV - pb->lvl_mV));
}
