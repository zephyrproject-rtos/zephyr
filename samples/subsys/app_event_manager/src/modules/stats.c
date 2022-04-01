/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>

#define MODULE stats

#include "measurement_event.h"

#include <logging/log.h>
#define STATS_LOG_LEVEL 4
LOG_MODULE_REGISTER(MODULE, STATS_LOG_LEVEL);

#define STATS_ARR_SIZE 10

static int32_t val_arr[STATS_ARR_SIZE];

static void add_to_stats(int32_t value)
{
	static size_t ind;

	/* Add new value */
	val_arr[ind] = value;
	ind++;

	if (ind == ARRAY_SIZE(val_arr)) {
		ind = 0;
		long long sum = 0;

		for (size_t i = 0; i < ARRAY_SIZE(val_arr); i++) {
			sum += val_arr[i];
		}
		LOG_INF("Average value3: %d", (int)(sum/ARRAY_SIZE(val_arr)));
	}
}

static bool event_handler(const struct event_header *eh)
{
	if (is_measurement_event(eh)) {
		struct measurement_event *me = cast_measurement_event(eh);

		add_to_stats(me->value3);
		return false;
	}

	/* If event is unhandled, unsubscribe. */
	__ASSERT_NO_MSG(false);

	return false;
}

EVENT_LISTENER(MODULE, event_handler);
EVENT_SUBSCRIBE(MODULE, measurement_event);
