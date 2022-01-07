/*
 * Copyright (c) 2021 EPAM Systems
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef __XEN_EVENTS_H__
#define __XEN_EVENTS_H__

#include <xen/public/event_channel.h>

#include <kernel.h>

typedef void (*evtchn_cb_t)(void *priv);

struct event_channel_handle {
	evtchn_cb_t cb;
	void *priv;
};

typedef struct event_channel_handle evtchn_handle_t;

void notify_evtchn(evtchn_port_t port);
int bind_event_channel(evtchn_port_t port, evtchn_cb_t cb, void *data);
int unbind_event_channel(evtchn_port_t port);

int xen_events_init(void);

#endif /* __XEN_EVENTS_H__ */
