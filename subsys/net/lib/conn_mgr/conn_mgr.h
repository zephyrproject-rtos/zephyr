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

#define CMGR_IF_ST_UP			BIT(0)
#define CMGR_IF_ST_IPV6_SET		BIT(1)
#define CMGR_IF_ST_IPV6_DAD_OK		BIT(2)
#define CMGR_IF_ST_IPV4_SET		BIT(3)
#define CMGR_IF_EVT_REQ_DOWN		BIT(4)

#define CMGR_IF_ST_READY		BIT(14)
#define CMGR_IF_EVT_CHANGED		BIT(15)

#define CONN_MGR_IFACE_EVENTS_MASK	(NET_EVENT_IF_DOWN			| \
					 NET_EVENT_IF_UP			| \
					 NET_EVENT_IF_CONNECTIVITY_TIMEOUT	| \
					 NET_EVENT_IF_CONNECTIVITY_FATAL_ERROR)

#define CONN_MGR_IPV6_EVENTS_MASK	(NET_EVENT_IPV6_ADDR_ADD		| \
					 NET_EVENT_IPV6_ADDR_DEL		| \
					 NET_EVENT_IPV6_DAD_SUCCEED		| \
					 NET_EVENT_IPV6_DAD_FAILED)

#define CONN_MGR_IPV4_EVENTS_MASK	(NET_EVENT_IPV4_ADDR_ADD		| \
					 NET_EVENT_IPV4_ADDR_DEL)

#define CONN_MGR_IPV6_STATUS_MASK	(CMGR_IF_ST_IPV6_SET			| \
					 CMGR_IF_ST_IPV6_DAD_OK)

#define CONN_MGR_IPV4_STATUS_MASK	(CMGR_IF_ST_IPV4_SET)

extern struct k_sem conn_mgr_lock;

enum conn_mgr_state {
	CMGR_ST_DISCONNECTED	= 0,
	CMGR_ST_CONNECTED	= 1,
};

void conn_mgr_init_events_handler(void);

/**
 * @brief Bring all network interfaces admin-up.
 *
 * @retval 0 if all calls to net_if_up succeed.
 * @retval otherwise, the error code returned by the first failed call to net_if_up.
 */
int net_conn_mgr_all_if_up(void);

/**
 * @brief Bring all network interfaces admin-down.
 *
 * @retval 0 if all calls to net_if_down succeed.
 * @retval otherwise, the error code returned by the first failed call to net_if_down.
 */
int net_conn_mgr_all_if_down(void);

/**
 * @brief Bring all network interfaces admin-up and connect those that support connection.
 *	  Does not call net_if_up on network interfaces that are already admin-up.
 *	  Calls net_if_up on all network ifaces, even those without net_if_connect support.
 *
 * @retval 0 if all calls to net_if_up and net_if_connect succeed.
 * @retval otherwise, the error code returned by the first failed call to net_if_up or
 *	   net_if_connect.
 */
int net_conn_mgr_all_if_connect(void);

/**
 * @brief Disconnect all network interfaces without bringing them admin-down.
 *
 * @retval 0 if all calls to net_if_disconnect succeed.
 * @retval otherwise, the error code returned by the first failed call to net_if_disconnect.
 */
int net_conn_mgr_all_if_disconnect(void);



#endif /* __CONN_MGR_H__ */
