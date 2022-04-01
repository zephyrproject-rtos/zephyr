/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>

#include "config_event.h"


static void log_config_event(const struct app_event_header *aeh)
{
	struct config_event *event = cast_config_event(aeh);

	APP_EVENT_MANAGER_LOG(aeh, "init_val_1=%d", event->init_value1);
}

APP_EVENT_TYPE_DEFINE(config_event,
		  log_config_event,
		  NULL,
		  APP_EVENT_FLAGS_CREATE(APP_EVENT_TYPE_FLAGS_INIT_LOG_ENABLE));
