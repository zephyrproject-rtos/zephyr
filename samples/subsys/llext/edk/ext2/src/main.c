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
	int i;

	for (i = 0; i < 5; i++) {
		struct channel_tick_data ctd = { .l = i };

		printk("[ext2]Publishing tick\n");
		publish(CHAN_TICK, &ctd, sizeof(ctd));
		k_sleep(K_SECONDS(1));
	}

	return 0;
}
EXPORT_SYMBOL(start);
