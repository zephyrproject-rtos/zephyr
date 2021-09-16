/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>

#include "measurement_event.h"

static void log_measurement_event(const struct event_header *eh)
{
	struct measurement_event *event = cast_measurement_event(eh);

	EVENT_MANAGER_LOG(eh, "val1=%d val2=%d val3=%d", event->value1,
			event->value2, event->value3);
}


EVENT_TYPE_DEFINE(measurement_event,
		  true,
		  log_measurement_event,
		  NULL);
