/*
 * Copyright (c) 2016 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#if defined(CONFIG_NET_DEBUG_L2_WIFI_MGMT)
#define SYS_LOG_DOMAIN "net/wifi_mgmt"
#define NET_LOG_ENABLED 1
#endif

#include <errno.h>

#include <net/net_core.h>
#include <net/net_if.h>
#include <net/wifi_mgmt.h>

static int wifi_connect(u32_t mgmt_request, struct net_if *iface,
			void *data, size_t len)
{
	return -ENETDOWN;
}

NET_MGMT_REGISTER_REQUEST_HANDLER(NET_REQUEST_WIFI_CONNECT, wifi_connect);


static int wifi_scan(u32_t mgmt_request, struct net_if *iface,
		     void *data, size_t len)
{
	return -ENETDOWN;
}

NET_MGMT_REGISTER_REQUEST_HANDLER(NET_REQUEST_WIFI_SCAN, wifi_scan);


static int wifi_disconnect(u32_t mgmt_request, struct net_if *iface,
			   void *data, size_t len)
{
	return -ENETDOWN;
}

NET_MGMT_REGISTER_REQUEST_HANDLER(NET_REQUEST_WIFI_DISCONNECT, wifi_disconnect);
