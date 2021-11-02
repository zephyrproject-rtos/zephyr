/*
 * Copyright (c) 2021 EPAM Systems
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef __XEN_EVENTS_H__
#define __XEN_EVENTS_H__

#include <xen/public/event_channel.h>

void notify_evtchn(evtchn_port_t port);

#endif /* __XEN_EVENTS_H__ */
