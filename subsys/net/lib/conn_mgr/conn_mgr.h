/*
 * Copyright (c) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __CONN_MGR_H__
#define __CONN_MGR_H__

#if defined(CONFIG_NET_IPV6) && defined(CONFIG_NET_IPV4)
#define CONN_MGR_IFACE_MAX		MAX(CONFIG_NET_IF_MAX_IPV6_COUNT, \
					    CONFIG_NET_IF_MAX_IPV4_COUNT)
#elif defined(CONFIG_NET_IPV6)
#define CONN_MGR_IFACE_MAX		CONFIG_NET_IF_MAX_IPV6_COUNT
#else
#define CONN_MGR_IFACE_MAX		CONFIG_NET_IF_MAX_IPV4_COUNT
#endif

#define NET_STATE_IFACE_UP		BIT(0)
#define NET_STATE_IPV6_ADDR_SET		BIT(1)
#define NET_STATE_IPV6_DAD_OK		BIT(2)
#define NET_STATE_IPV4_ADDR_SET		BIT(3)

#define NET_STATE_CONNECTED		BIT(14)
#define NET_STATE_CHANGED		BIT(15)

#define CONN_MGR_IFACE_EVENTS_MASK	(NET_EVENT_IF_DOWN          |	\
					 NET_EVENT_IF_UP)

#define CONN_MGR_IPV6_EVENTS_MASK	(NET_EVENT_IPV6_ADDR_ADD    |	\
					 NET_EVENT_IPV6_ADDR_DEL    |	\
					 NET_EVENT_IPV6_DAD_SUCCEED |	\
					 NET_EVENT_IPV6_DAD_FAILED)

#define CONN_MGR_IPV4_EVENTS_MASK	(NET_EVENT_IPV4_ADDR_ADD    |	\
					 NET_EVENT_IPV4_ADDR_DEL)

#define CONN_MGR_IPV6_STATUS_MASK	(NET_STATE_IPV6_ADDR_SET    |	\
					 NET_STATE_IPV6_DAD_OK)

#define CONN_MGR_IPV4_STATUS_MASK	(NET_STATE_IPV4_ADDR_SET)

extern struct k_sem conn_mgr_lock;

enum net_conn_mgr_state {
	NET_CONN_MGR_STATE_DISCONNECTED	= 0,
	NET_CONN_MGR_STATE_CONNECTED	= 1,
};

void conn_mgr_init_events_handler(void);

#endif /* __CONN_MGR_H__ */
