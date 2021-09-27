/*
 * Copyright (c) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "wasm_app.h"
#include "wa-inc/timer_wasm_app.h"

/* User global variable */
static int num;

/* Timer callback */
void timer1_update(user_timer_t timer)
{
	printf("Timer update %d\n", num++);
}

void on_init(void)
{
	user_timer_t timer;

	/* set up a timer */
	timer = api_timer_create(1000, true, false, timer1_update);
	api_timer_restart(timer, 1000);
}

void on_destroy(void)
{
	/**
	 * real destroy work including killing timer and closing sensor is
	 * accomplished in wasm app library version of on_destroy()
	 */
}
