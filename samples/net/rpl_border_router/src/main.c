/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#if 1
#define SYS_LOG_DOMAIN "rpl-br/main"
#define NET_SYS_LOG_LEVEL SYS_LOG_LEVEL_DEBUG
#define NET_LOG_ENABLED 1
#endif

#include <zephyr.h>
#include <stdio.h>

#include <net/net_if.h>

#include "config.h"

/* Sets the network parameters */

void main(void)
{
	struct net_if *iface = NULL, *mgmt_iface = NULL;

	NET_DBG("RPL border router starting");

#if defined(CONFIG_NET_L2_IEEE802154)
	iface = net_if_get_ieee802154();
	if (!iface) {
		NET_INFO("No IEEE 802.15.4 network interface found.");
	}
#else
#if defined(CONFIG_QEMU_TARGET)
	/* This is just for testing purposes, the RPL network
	 * is not fully functional with QEMU.
	 */
	iface = net_if_get_default();
#endif
#endif

	if (!iface) {
		NET_INFO("Cannot continue because no suitable network "
			 "interface exists.");
		return;
	}

	mgmt_iface = net_if_get_first_by_type(&NET_L2_GET_NAME(ETHERNET));
	if (!mgmt_iface) {
		NET_INFO("No management network interface found.");
	} else {
		start_http_server(mgmt_iface);
	}

	setup_rpl(iface, CONFIG_NET_RPL_PREFIX);

#if defined(CONFIG_COAP)
	coap_init();
#endif
}
