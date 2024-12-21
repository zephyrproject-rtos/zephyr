/*
 * Copyright (c) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __CONN_MGR_PRV_H__
#define __CONN_MGR_PRV_H__

#include <zephyr/net/conn_mgr_connectivity.h>
#include <zephyr/sys/iterable_sections.h>

#if defined(CONFIG_NET_IPV6) && defined(CONFIG_NET_IPV4)
#define CONN_MGR_IFACE_MAX		MAX(CONFIG_NET_IF_MAX_IPV6_COUNT, \
					    CONFIG_NET_IF_MAX_IPV4_COUNT)
#elif defined(CONFIG_NET_IPV6)
#define CONN_MGR_IFACE_MAX		CONFIG_NET_IF_MAX_IPV6_COUNT
#else
#define CONN_MGR_IFACE_MAX		CONFIG_NET_IF_MAX_IPV4_COUNT
#endif

/* External state flags */
#define CONN_MGR_IF_UP			BIT(0)
#define CONN_MGR_IF_IPV6_SET		BIT(1)
#define CONN_MGR_IF_IPV4_SET		BIT(2)

/* Configuration flags */
#define CONN_MGR_IF_IGNORED		BIT(7)

/* Internal state flags */
#define CONN_MGR_IF_READY		BIT(13)
#define CONN_MGR_IF_READY_IPV4		BIT(14)
#define CONN_MGR_IF_READY_IPV6		BIT(15)

/* Special value indicating invalid state. */
#define CONN_MGR_IF_STATE_INVALID	0xFFFF

/* NET_MGMT event masks */
#define CONN_MGR_IFACE_EVENTS_MASK	(NET_EVENT_IF_DOWN		| \
					 NET_EVENT_IF_UP)

#define CONN_MGR_CONN_IFACE_EVENTS_MASK	(NET_EVENT_IF_ADMIN_UP		|\
					 NET_EVENT_IF_DOWN)

#define CONN_MGR_CONN_SELF_EVENTS_MASK	(NET_EVENT_CONN_IF_TIMEOUT	| \
					 NET_EVENT_CONN_IF_FATAL_ERROR)

#define CONN_MGR_IPV6_EVENTS_MASK	(NET_EVENT_IPV6_ADDR_ADD	| \
					 NET_EVENT_IPV6_ADDR_DEL	| \
					 NET_EVENT_IPV6_DAD_SUCCEED	| \
					 NET_EVENT_IPV6_DAD_FAILED)

#define CONN_MGR_IPV4_EVENTS_MASK	(NET_EVENT_IPV4_ADDR_ADD	| \
					 NET_EVENT_IPV4_ADDR_DEL	| \
					 NET_EVENT_IPV4_ACD_SUCCEED	| \
					 NET_EVENT_IPV4_ACD_FAILED)

extern struct k_sem conn_mgr_mon_updated;
extern struct k_mutex conn_mgr_mon_lock;

void conn_mgr_init_events_handler(void);

/* Cause conn_mgr_connectivity to Initialize all connectivity implementation bindings */
void conn_mgr_conn_init(void);

/* Internal helper function to allow the shell net cm command to safely read conn_mgr state. */
uint16_t conn_mgr_if_state(struct net_if *iface);

#endif /* __CONN_MGR_PRV_H__ */
