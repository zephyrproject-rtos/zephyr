/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * Copyright (c) 2020 Philippe Gerum <rpm@xenomai.org>
 */

#ifndef _EVL_ZEPHYR_LATMON_H
#define _EVL_ZEPHYR_LATMON_H

#include <stdint.h>

#define LATMON_NET_PORT 2306

struct latmon_net_request {
	uint32_t period_usecs;
	uint32_t histogram_cells;
} __attribute__((__packed__));

struct latmon_net_data {
	int32_t sum_lat_hi;
	int32_t sum_lat_lo;
	int32_t min_lat;
	int32_t max_lat;
	uint32_t overruns;
	uint32_t samples;
} __attribute__((__packed__));

#endif /* !_EVL_ZEPHYR_LATMON_H */
