/*
 * Copyright (c) 2024 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/llext/symbol.h>

#include <app_api.h>

int start(void)
{
	/* Inside extensions, all kobjects need to be dynamically allocated */
	struct k_event *tick_evt = k_object_alloc(K_OBJ_EVENT);

	k_event_init(tick_evt);

	register_subscriber(CHAN_TICK, tick_evt);

	while (true) {
		long l;

		printk("[ext1]Waiting event\n");
		k_event_wait(tick_evt, CHAN_TICK, true, K_FOREVER);

		printk("[ext1]Got event, reading channel\n");
		receive(CHAN_TICK, &l, sizeof(l));
		printk("[ext1]Read val: %ld\n", l);
	}

	return 0;
}
LL_EXTENSION_SYMBOL(start);
