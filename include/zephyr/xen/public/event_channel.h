/* SPDX-License-Identifier: MIT */

/******************************************************************************
 * event_channel.h
 *
 * Event channels between domains.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 * Copyright (c) 2003-2004, K A Fraser.
 */

#ifndef __XEN_PUBLIC_EVENT_CHANNEL_H__
#define __XEN_PUBLIC_EVENT_CHANNEL_H__

#include "xen.h"

/*
 * `incontents 150 evtchn Event Channels
 *
 * Event channels are the basic primitive provided by Xen for event
 * notifications. An event is the Xen equivalent of a hardware
 * interrupt. They essentially store one bit of information, the event
 * of interest is signalled by transitioning this bit from 0 to 1.
 *
 * Notifications are received by a guest via an upcall from Xen,
 * indicating when an event arrives (setting the bit). Further
 * notifications are masked until the bit is cleared again (therefore,
 * guests must check the value of the bit after re-enabling event
 * delivery to ensure no missed notifications).
 *
 * Event notifications can be masked by setting a flag; this is
 * equivalent to disabling interrupts and can be used to ensure
 * atomicity of certain operations in the guest kernel.
 *
 * Event channels are represented by the evtchn_* fields in
 * struct shared_info and struct vcpu_info.
 */

#define EVTCHNOP_bind_interdomain	0
#define EVTCHNOP_bind_virq		1
#define EVTCHNOP_bind_pirq		2
#define EVTCHNOP_close			3
#define EVTCHNOP_send			4
#define EVTCHNOP_status			5
#define EVTCHNOP_alloc_unbound		6
#define EVTCHNOP_bind_ipi		7
#define EVTCHNOP_bind_vcpu		8
#define EVTCHNOP_unmask			9
#define EVTCHNOP_reset			10
#define EVTCHNOP_init_control		11
#define EVTCHNOP_expand_array		12
#define EVTCHNOP_set_priority		13
#ifdef __XEN__
#define EVTCHNOP_reset_cont		14
#endif

typedef uint32_t evtchn_port_t;
DEFINE_XEN_GUEST_HANDLE(evtchn_port_t);

/*
 * EVTCHNOP_alloc_unbound: Allocate a port in domain <dom> and mark as
 * accepting interdomain bindings from domain <remote_dom>. A fresh port
 * is allocated in <dom> and returned as <port>.
 * NOTES:
 *  1. If the caller is unprivileged then <dom> must be DOMID_SELF.
 *  2. <remote_dom> may be DOMID_SELF, allowing loopback connections.
 */
struct evtchn_alloc_unbound {
	/* IN parameters */
	domid_t dom, remote_dom;
	/* OUT parameters */
	evtchn_port_t port;
};
typedef struct evtchn_alloc_unbound evtchn_alloc_unbound_t;

/*
 * EVTCHNOP_bind_interdomain: Construct an interdomain event channel between
 * the calling domain and <remote_dom>. <remote_dom,remote_port> must identify
 * a port that is unbound and marked as accepting bindings from the calling
 * domain. A fresh port is allocated in the calling domain and returned as
 * <local_port>.
 *
 * In case the peer domain has already tried to set our event channel
 * pending, before it was bound, EVTCHNOP_bind_interdomain always sets
 * the local event channel pending.
 *
 * The usual pattern of use, in the guest's upcall (or subsequent
 * handler) is as follows: (Re-enable the event channel for subsequent
 * signalling and then) check for the existence of whatever condition
 * is being waited for by other means, and take whatever action is
 * needed (if any).
 *
 * NOTES:
 *  1. <remote_dom> may be DOMID_SELF, allowing loopback connections.
 */
struct evtchn_bind_interdomain {
	/* IN parameters. */
	domid_t remote_dom;
	evtchn_port_t remote_port;
	/* OUT parameters. */
	evtchn_port_t local_port;
};
typedef struct evtchn_bind_interdomain evtchn_bind_interdomain_t;

/*
 * EVTCHNOP_close: Close a local event channel <port>. If the channel is
 * interdomain then the remote end is placed in the unbound state
 * (EVTCHNSTAT_unbound), awaiting a new connection.
 */
struct evtchn_close {
	/* IN parameters. */
	evtchn_port_t port;
};
typedef struct evtchn_close evtchn_close_t;

/*
 * EVTCHNOP_send: Send an event to the remote end of the channel whose local
 * endpoint is <port>.
 */
struct evtchn_send {
	/* IN parameters. */
	evtchn_port_t port;
};
typedef struct evtchn_send evtchn_send_t;

/*
 * EVTCHNOP_status: Get the current status of the communication channel which
 * has an endpoint at <dom, port>.
 * NOTES:
 *  1. <dom> may be specified as DOMID_SELF.
 *  2. Only a sufficiently-privileged domain may obtain the status of an event
 *     channel for which <dom> is not DOMID_SELF.
 */
struct evtchn_status {
	/* IN parameters */
	domid_t  dom;
	evtchn_port_t port;
	/* OUT parameters */
#define EVTCHNSTAT_closed	0 /* Channel is not in use.                 */
#define EVTCHNSTAT_unbound	1 /* Channel is waiting interdom connection.*/
#define EVTCHNSTAT_interdomain	2 /* Channel is connected to remote domain. */
#define EVTCHNSTAT_pirq		3 /* Channel is bound to a phys IRQ line.   */
#define EVTCHNSTAT_virq		4 /* Channel is bound to a virtual IRQ line */
#define EVTCHNSTAT_ipi		5 /* Channel is bound to a virtual IPI line */
	uint32_t status;
	uint32_t vcpu;			/* VCPU to which this channel is bound.   */
	union {
		struct {
			domid_t dom;
		} unbound;			/* EVTCHNSTAT_unbound */
		struct {
			domid_t dom;
			evtchn_port_t port;
		} interdomain;			/* EVTCHNSTAT_interdomain */
		uint32_t pirq;			/* EVTCHNSTAT_pirq */
		uint32_t virq;			/* EVTCHNSTAT_virq */
	} u;
};
typedef struct evtchn_status evtchn_status_t;

/*
 * EVTCHNOP_unmask: Unmask the specified local event-channel port and deliver
 * a notification to the appropriate VCPU if an event is pending.
 */
struct evtchn_unmask {
	/* IN parameters. */
	evtchn_port_t port;
};
typedef struct evtchn_unmask evtchn_unmask_t;

/*
 * EVTCHNOP_reset: Close all event channels associated with specified domain.
 * NOTES:
 *  1. <dom> may be specified as DOMID_SELF.
 *  2. Only a sufficiently-privileged domain may specify other than DOMID_SELF.
 *  3. Destroys all control blocks and event array, resets event channel
 *     operations to 2-level ABI if called with <dom> == DOMID_SELF and FIFO
 *     ABI was used. Guests should not bind events during EVTCHNOP_reset call
 *     as these events are likely to be lost.
 */
struct evtchn_reset {
	/* IN parameters. */
	domid_t dom;
};
typedef struct evtchn_reset evtchn_reset_t;

/*
 * EVTCHNOP_set_priority: set the priority for an event channel.
 */
struct evtchn_set_priority {
	/* IN parameters. */
	evtchn_port_t port;
	uint32_t priority;
};
typedef struct evtchn_set_priority evtchn_set_priority_t;

#define EVTCHN_2L_NR_CHANNELS (sizeof(xen_ulong_t) * sizeof(xen_ulong_t) * 64)

#endif /* __XEN_PUBLIC_EVENT_CHANNEL_H__ */
