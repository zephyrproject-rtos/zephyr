/** @file
 * @brief Generic connection manager related routines.
 */

/*
 * Copyright (c) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <logging/log.h>
LOG_MODULE_REGISTER(net_conn_mgr, CONFIG_NET_CONNECTION_MANAGER_LOG_LEVEL);

#include <net/net_core.h>
#include <net/net_ip.h>
#include <net/net_if.h>
#include <net/net_mgmt.h>

/*
 * TODO:
 *
 * - Create a monitor that could periodically try to check if the connectivity
 *   is still valid. How this period should be done is the tricky part.
 *
 * - Try to establish a connection to pre-configured address in order to make
 *   sure that we really have a working network connection. The connectivity
 *   check might tell us that we have only local network connectivity or if
 *   we have a full Internet connectivity.
 */

void net_conn_mgr_connect(struct net_if *iface, sa_family_t family)
{
	/* We have IP adderess added to the system, so possibly we have
	 * a proper network connection established atm.
	 */
	if (net_if_flag_is_set(iface, NET_IF_CONNECTED)) {
		return;
	}

	net_if_flag_set(iface, NET_IF_CONNECTED);

	NET_DBG("Iface %p family %s %s", iface, net_family2str(family),
		"connected");

	net_mgmt_event_notify(NET_EVENT_L4_CONNECTED, iface);
}

void net_conn_mgr_disconnect(struct net_if *iface, sa_family_t family)
{
	ARG_UNUSED(family);

	if (!net_if_flag_is_set(iface, NET_IF_CONNECTED)) {
		return;
	}

	net_if_flag_clear(iface, NET_IF_CONNECTED);

	NET_DBG("Iface %p family %s %s", iface, net_family2str(family),
		"disconnected");

	net_mgmt_event_notify(NET_EVENT_L4_DISCONNECTED, iface);
}
