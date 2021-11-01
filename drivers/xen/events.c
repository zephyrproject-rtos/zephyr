/*
 * Copyright (c) 2021 EPAM Systems
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <arch/arm64/hypercall.h>
#include <xen/public/xen.h>
#include <xen/public/event_channel.h>
#include <xen/events.h>

void notify_evtchn(evtchn_port_t port)
{
	struct evtchn_send send;

	if (port >= EVTCHN_2L_NR_CHANNELS) {
		printk("%s: trying to send notify for invalid evtchn #%u\n",
				__func__, port);
		return;
	}

	send.port = port;

	HYPERVISOR_event_channel_op(EVTCHNOP_send, &send);
}
