/*
 * Copyright 2023 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/sw_isr_table.h>

/* an interrupt line can be considered shared only if there's
 * at least 2 clients using it. As such, enforce the fact that
 * the maximum number of allowed clients should be at least 2.
 */
BUILD_ASSERT(CONFIG_SHARED_IRQ_MAX_NUM_CLIENTS >= 2,
	     "maximum number of clients should be at least 2");

void z_shared_isr(const void *data)
{
	size_t i;
	const struct z_shared_isr_table_entry *entry;
	const struct z_shared_isr_client *client;

	entry = data;

	for (i = 0; i < entry->client_num; i++) {
		client = &entry->clients[i];

		if (client->isr) {
			client->isr(client->arg);
		}
	}
}
