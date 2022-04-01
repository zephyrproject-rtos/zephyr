/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <event_manager/event_manager.h>
#include <config_event.h>

#include <logging/log.h>
LOG_MODULE_REGISTER(MODULE);

#define INIT_VALUE1 3

void main(void)
{
	if (event_manager_init()) {
		LOG_ERR("Event Manager not initialized");
	} else {
		struct config_event *event = new_config_event();

		event->init_value1 = INIT_VALUE1;
		EVENT_SUBMIT(event);
	}
}
