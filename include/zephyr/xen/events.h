/*
 * Copyright (c) 2021 EPAM Systems
 * Copyright (c) 2022 Arm Limited (or its affiliates). All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef __XEN_EVENTS_H__
#define __XEN_EVENTS_H__

#include <zephyr/xen/public/event_channel.h>

#include <zephyr/kernel.h>

typedef void (*evtchn_cb_t)(void *priv);

struct event_channel_handle {
	evtchn_cb_t cb;
	void *priv;
};
typedef struct event_channel_handle evtchn_handle_t;

/*
 * Following functions just wrap Xen hypercalls, detailed description
 * of parameters and return values are located in include/xen/public/event_channel.h
 */
int evtchn_status(evtchn_status_t *status);
int evtchn_close(evtchn_port_t port);
int evtchn_set_priority(evtchn_port_t port, uint32_t priority);
void notify_evtchn(evtchn_port_t port);

/*
 * Allocate event-channel between caller and remote domain
 *
 * @param remote_dom - remote domain domid
 * @return - local event channel port on success, negative on error
 */
int alloc_unbound_event_channel(domid_t remote_dom);

/*
 * Allocate local event channel, binded to remote port and attach specified callback
 * to it
 *
 * @param remote_dom - remote domain domid
 * @param remote_port - remote domain event channel port number
 * @param cb - callback, attached to locat port
 * @param data - private data, that will be passed to cb
 * @return - local event channel port on success, negative on error
 */
int bind_interdomain_event_channel(domid_t remote_dom, evtchn_port_t remote_port,
		evtchn_cb_t cb, void *data);

/*
 * Bind user-defined handler to specified event-channel
 *
 * @param port - event channel number
 * @param cb - pointer to event channel handler
 * @param data - private data, that will be passed to handler as parameter
 * @return - zero on success
 */
int bind_event_channel(evtchn_port_t port, evtchn_cb_t cb, void *data);

/*
 * Unbind handler from event channel, substitute it with empty callback
 *
 * @param port - event channel number to unbind
 * @return - zero on success
 */
int unbind_event_channel(evtchn_port_t port);
int get_missed_events(evtchn_port_t port);

int xen_events_init(void);

#endif /* __XEN_EVENTS_H__ */
