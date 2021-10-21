/*
 * Copyright (c) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "wasm_app.h"
#include "wa-inc/request.h"
#include "wa-inc/timer_wasm_app.h"

void publish_overheat_event(void)
{
	attr_container_t *event;
	int len;

	event = attr_container_create("event");
	attr_container_set_string(&event, "warning",
							  "temperature is over high");

	len = attr_container_get_serialize_length(event);
	api_publish_event("alert/overheat", FMT_ATTR_CONTAINER,
					  event, len);

	attr_container_destroy(event);
}

/* Timer callback */
void timer1_update(user_timer_t timer)
{
	publish_overheat_event();
}

void start_timer(void)
{
	user_timer_t timer;

	/* set up a timer */
	timer = api_timer_create(1000, true, false, timer1_update);
	api_timer_restart(timer, 1000);
}

void on_init(void)
{
	start_timer();
}

void on_destroy(void)
{
	/**
	 * real destroy work including killing timer and closing sensor
	 * is accomplished in wasm app library version of on_destroy()
	 */
}
