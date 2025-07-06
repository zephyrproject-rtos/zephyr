/*
 * Copyright (c) 2024 Trackunit Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/modem/pipelink.h>

static void try_callback(struct modem_pipelink *link, enum modem_pipelink_event event)
{
	if (link->callback == NULL) {
		return;
	}

	link->callback(link, event, link->user_data);
}

void modem_pipelink_attach(struct modem_pipelink *link,
			   modem_pipelink_callback callback,
			   void *user_data)
{
	K_SPINLOCK(&link->spinlock) {
		link->callback = callback;
		link->user_data = user_data;
	}
}

bool modem_pipelink_is_connected(struct modem_pipelink *link)
{
	bool connected;

	K_SPINLOCK(&link->spinlock) {
		connected = link->connected;
	}

	return connected;
}

struct modem_pipe *modem_pipelink_get_pipe(struct modem_pipelink *link)
{
	return link->pipe;
}

void modem_pipelink_release(struct modem_pipelink *link)
{
	K_SPINLOCK(&link->spinlock) {
		link->callback = NULL;
		link->user_data = NULL;
	}
}

void modem_pipelink_init(struct modem_pipelink *link, struct modem_pipe *pipe)
{
	link->pipe = pipe;
	link->callback = NULL;
	link->user_data = NULL;
	link->connected = false;
}

void modem_pipelink_notify_connected(struct modem_pipelink *link)
{
	K_SPINLOCK(&link->spinlock) {
		if (link->connected) {
			K_SPINLOCK_BREAK;
		}

		link->connected = true;
		try_callback(link, MODEM_PIPELINK_EVENT_CONNECTED);
	}
}

void modem_pipelink_notify_disconnected(struct modem_pipelink *link)
{
	K_SPINLOCK(&link->spinlock) {
		if (!link->connected) {
			K_SPINLOCK_BREAK;
		}

		link->connected = false;
		try_callback(link, MODEM_PIPELINK_EVENT_DISCONNECTED);
	}
}
