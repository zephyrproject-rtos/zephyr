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
 * EVTCHNOP_send: Send an event to the remote end of the channel whose local
 * endpoint is <port>.
 */
struct evtchn_send {
	/* IN parameters. */
	evtchn_port_t port;
};
typedef struct evtchn_send evtchn_send_t;

#define EVTCHN_2L_NR_CHANNELS (sizeof(xen_ulong_t) * sizeof(xen_ulong_t) * 64)

#endif /* __XEN_PUBLIC_EVENT_CHANNEL_H__ */
