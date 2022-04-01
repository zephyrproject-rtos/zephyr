/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>

#include "measurement_event.h"

static void log_measurement_event(const struct app_event_header *aeh)
{
	struct measurement_event *event = cast_measurement_event(aeh);

	APP_EVENT_MANAGER_LOG(aeh, "val1=%d val2=%d val3=%d", event->value1,
			event->value2, event->value3);
}


APP_EVENT_TYPE_DEFINE(measurement_event,
		  log_measurement_event,
		  NULL,
		  APP_EVENT_FLAGS_CREATE(APP_EVENT_TYPE_FLAGS_INIT_LOG_ENABLE));
